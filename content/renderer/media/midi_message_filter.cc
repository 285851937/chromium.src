// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/midi_message_filter.h"

#include <algorithm>

#include "base/bind.h"
#include "base/debug/trace_event.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/media/midi_messages.h"
#include "content/renderer/render_thread_impl.h"
#include "ipc/ipc_logging.h"

using media::MidiPortInfoList;
using base::AutoLock;

// The maximum number of bytes which we're allowed to send to the browser
// before getting acknowledgement back from the browser that they've been
// successfully sent.
static const size_t kMaxUnacknowledgedBytesSent = 10 * 1024 * 1024;  // 10 MB.

namespace content {

// TODO(crbug.com/425389): Rewrite this class as a RenderFrameObserver.
MidiMessageFilter::MidiMessageFilter(
    const scoped_refptr<base::MessageLoopProxy>& io_message_loop)
    : sender_(NULL),
      io_message_loop_(io_message_loop),
      main_message_loop_(base::MessageLoopProxy::current()),
      session_result_(media::MIDI_NOT_INITIALIZED),
      unacknowledged_bytes_sent_(0u) {
}

MidiMessageFilter::~MidiMessageFilter() {}

void MidiMessageFilter::AddClient(blink::WebMIDIAccessorClient* client) {
  DCHECK(main_message_loop_->BelongsToCurrentThread());
  TRACE_EVENT0("midi", "MidiMessageFilter::AddClient");
  clients_waiting_session_queue_.push_back(client);
  if (session_result_ != media::MIDI_NOT_INITIALIZED) {
    HandleClientAdded(session_result_);
  } else if (clients_waiting_session_queue_.size() == 1u) {
    io_message_loop_->PostTask(FROM_HERE,
        base::Bind(&MidiMessageFilter::StartSessionOnIOThread, this));
  }
}

void MidiMessageFilter::RemoveClient(blink::WebMIDIAccessorClient* client) {
  DCHECK(main_message_loop_->BelongsToCurrentThread());
  clients_.erase(client);
  ClientsQueue::iterator it = std::find(clients_waiting_session_queue_.begin(),
                                        clients_waiting_session_queue_.end(),
                                        client);
  if (it != clients_waiting_session_queue_.end())
    clients_waiting_session_queue_.erase(it);
  if (clients_.empty() && clients_waiting_session_queue_.empty()) {
    session_result_ = media::MIDI_NOT_INITIALIZED;
    io_message_loop_->PostTask(FROM_HERE,
        base::Bind(&MidiMessageFilter::EndSessionOnIOThread, this));
  }
}

void MidiMessageFilter::SendMidiData(uint32 port,
                                     const uint8* data,
                                     size_t length,
                                     double timestamp) {
  DCHECK(main_message_loop_->BelongsToCurrentThread());
  if ((kMaxUnacknowledgedBytesSent - unacknowledged_bytes_sent_) < length) {
    // TODO(toyoshim): buffer up the data to send at a later time.
    // For now we're just dropping these bytes on the floor.
    return;
  }

  unacknowledged_bytes_sent_ += length;
  std::vector<uint8> v(data, data + length);
  io_message_loop_->PostTask(FROM_HERE, base::Bind(
        &MidiMessageFilter::SendMidiDataOnIOThread, this, port, v, timestamp));
}

void MidiMessageFilter::StartSessionOnIOThread() {
  TRACE_EVENT0("midi", "MidiMessageFilter::StartSessionOnIOThread");
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  Send(new MidiHostMsg_StartSession());
}

void MidiMessageFilter::SendMidiDataOnIOThread(uint32 port,
                                               const std::vector<uint8>& data,
                                               double timestamp) {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  Send(new MidiHostMsg_SendData(port, data, timestamp));
}

void MidiMessageFilter::EndSessionOnIOThread() {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  Send(new MidiHostMsg_EndSession());
}

void MidiMessageFilter::Send(IPC::Message* message) {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  if (!sender_) {
    delete message;
  } else {
    sender_->Send(message);
  }
}

bool MidiMessageFilter::OnMessageReceived(const IPC::Message& message) {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(MidiMessageFilter, message)
    IPC_MESSAGE_HANDLER(MidiMsg_SessionStarted, OnSessionStarted)
    IPC_MESSAGE_HANDLER(MidiMsg_DataReceived, OnDataReceived)
    IPC_MESSAGE_HANDLER(MidiMsg_AcknowledgeSentData, OnAcknowledgeSentData)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void MidiMessageFilter::OnFilterAdded(IPC::Sender* sender) {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  sender_ = sender;
}

void MidiMessageFilter::OnFilterRemoved() {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  // Once removed, a filter will not be used again.  At this time all
  // delegates must be notified so they release their reference.
  OnChannelClosing();
}

void MidiMessageFilter::OnChannelClosing() {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  sender_ = NULL;
}

void MidiMessageFilter::OnSessionStarted(media::MidiResult result,
                                         MidiPortInfoList inputs,
                                         MidiPortInfoList outputs) {
  TRACE_EVENT0("midi", "MidiMessageFilter::OnSessionStarted");
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  // TODO(toyoshim): |inputs_| and |outputs_| should not be updated on
  // |io_message_loop_|. This should be fixed in a following change not to
  // distribute MidiPortInfo via OnSessionStarted().
  // For now, this is safe because these are not updated later.
  inputs_ = inputs;
  outputs_ = outputs;
  // Handle on the main JS thread.
  main_message_loop_->PostTask(
      FROM_HERE,
      base::Bind(&MidiMessageFilter::HandleClientAdded, this, result));
}

void MidiMessageFilter::OnDataReceived(uint32 port,
                                       const std::vector<uint8>& data,
                                       double timestamp) {
  TRACE_EVENT0("midi", "MidiMessageFilter::OnDataReceived");
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  // Handle on the main JS thread.
  main_message_loop_->PostTask(
      FROM_HERE,
      base::Bind(&MidiMessageFilter::HandleDataReceived, this, port, data,
                 timestamp));
}

void MidiMessageFilter::OnAcknowledgeSentData(size_t bytes_sent) {
  DCHECK(io_message_loop_->BelongsToCurrentThread());
  main_message_loop_->PostTask(
      FROM_HERE,
      base::Bind(&MidiMessageFilter::HandleAckknowledgeSentData, this,
                 bytes_sent));
}

void MidiMessageFilter::HandleClientAdded(media::MidiResult result) {
  TRACE_EVENT0("midi", "MidiMessageFilter::HandleClientAdded");
  DCHECK(main_message_loop_->BelongsToCurrentThread());
  session_result_ = result;
  std::string error;
  std::string message;
  switch (result) {
    case media::MIDI_OK:
      break;
    case media::MIDI_NOT_SUPPORTED:
      error = "NotSupportedError";
      break;
    case media::MIDI_INITIALIZATION_ERROR:
      error = "InvalidStateError";
      message = "Platform dependent initialization failed.";
      break;
    default:
      NOTREACHED();
      error = "InvalidStateError";
      message = "Unknown internal error occurred.";
      break;
  }
  base::string16 error16 = base::UTF8ToUTF16(error);
  base::string16 message16 = base::UTF8ToUTF16(message);
  for (blink::WebMIDIAccessorClient* client : clients_waiting_session_queue_) {
    if (result == media::MIDI_OK) {
      // Add the client's input and output ports.
      const bool active = true;
      for (const auto& info : inputs_) {
        client->didAddInputPort(
            base::UTF8ToUTF16(info.id),
            base::UTF8ToUTF16(info.manufacturer),
            base::UTF8ToUTF16(info.name),
            base::UTF8ToUTF16(info.version),
            active);
      }

      for (const auto& info : outputs_) {
        client->didAddOutputPort(
            base::UTF8ToUTF16(info.id),
            base::UTF8ToUTF16(info.manufacturer),
            base::UTF8ToUTF16(info.name),
            base::UTF8ToUTF16(info.version),
            active);
      }
    }
    client->didStartSession(result == media::MIDI_OK, error16, message16);
    clients_.insert(client);
  }
  clients_waiting_session_queue_.clear();
}

void MidiMessageFilter::HandleDataReceived(uint32 port,
                                           const std::vector<uint8>& data,
                                           double timestamp) {
  TRACE_EVENT0("midi", "MidiMessageFilter::HandleDataReceived");
  DCHECK(main_message_loop_->BelongsToCurrentThread());
  DCHECK(!data.empty());

  for (blink::WebMIDIAccessorClient* client : clients_)
    client->didReceiveMIDIData(port, &data[0], data.size(), timestamp);
}

void MidiMessageFilter::HandleAckknowledgeSentData(size_t bytes_sent) {
  DCHECK(main_message_loop_->BelongsToCurrentThread());
  DCHECK_GE(unacknowledged_bytes_sent_, bytes_sent);
  if (unacknowledged_bytes_sent_ >= bytes_sent)
    unacknowledged_bytes_sent_ -= bytes_sent;
}

}  // namespace content
