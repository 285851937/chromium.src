// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/gpu_video_common.h"

namespace IPC {

void ParamTraits<GpuVideoServiceInfoParam>::Write(
    Message* m, const GpuVideoServiceInfoParam& p) {
  WriteParam(m, p.video_service_route_id);
  WriteParam(m, p.video_service_host_route_id);
  WriteParam(m, p.service_available);
}

bool ParamTraits<GpuVideoServiceInfoParam>::Read(
    const Message* m, void** iter, GpuVideoServiceInfoParam* r) {
  if (!ReadParam(m, iter, &r->video_service_route_id) ||
      !ReadParam(m, iter, &r->video_service_host_route_id) ||
      !ReadParam(m, iter, &r->service_available))
    return false;
  return true;
}

void ParamTraits<GpuVideoServiceInfoParam>::Log(
    const GpuVideoServiceInfoParam& p, std::string* l) {
  l->append(StringPrintf("(%d, %d, %d)",
            p.video_service_route_id,
            p.video_service_host_route_id,
            p.service_available));
}

///////////////////////////////////////////////////////////////////////////////

void ParamTraits<GpuVideoDecoderInfoParam>::Write(
    Message* m, const GpuVideoDecoderInfoParam& p) {
  WriteParam(m, p.context_id);
  WriteParam(m, p.decoder_id);
  WriteParam(m, p.decoder_route_id);
  WriteParam(m, p.decoder_host_route_id);
}

bool ParamTraits<GpuVideoDecoderInfoParam>::Read(
    const Message* m, void** iter, GpuVideoDecoderInfoParam* r) {
  if (!ReadParam(m, iter, &r->context_id) ||
      !ReadParam(m, iter, &r->decoder_id) ||
      !ReadParam(m, iter, &r->decoder_route_id) ||
      !ReadParam(m, iter, &r->decoder_host_route_id))
    return false;
  return true;
}

void ParamTraits<GpuVideoDecoderInfoParam>::Log(
    const GpuVideoDecoderInfoParam& p, std::string* l) {
  l->append(StringPrintf("(%d, %d, %d)",
            p.decoder_id,
            p.decoder_route_id,
            p.decoder_host_route_id));
}

///////////////////////////////////////////////////////////////////////////////

void ParamTraits<GpuVideoDecoderInitParam>::Write(
    Message* m, const GpuVideoDecoderInitParam& p) {
  WriteParam(m, p.codec_id);
  WriteParam(m, p.width);
  WriteParam(m, p.height);
}

bool ParamTraits<GpuVideoDecoderInitParam>::Read(
    const Message* m, void** iter, GpuVideoDecoderInitParam* r) {
  if (!ReadParam(m, iter, &r->codec_id) ||
      !ReadParam(m, iter, &r->width) ||
      !ReadParam(m, iter, &r->height))
    return false;
  return true;
}

void ParamTraits<GpuVideoDecoderInitParam>::Log(
    const GpuVideoDecoderInitParam& p, std::string* l) {
  l->append(StringPrintf("(%d, %d %d)", p.codec_id, p.width, p.height));
}

///////////////////////////////////////////////////////////////////////////////

void ParamTraits<GpuVideoDecoderInitDoneParam>::Write(
    Message* m, const GpuVideoDecoderInitDoneParam& p) {
  WriteParam(m, p.success);
  WriteParam(m, p.input_buffer_size);
  WriteParam(m, p.input_buffer_handle);
}

bool ParamTraits<GpuVideoDecoderInitDoneParam>::Read(
    const Message* m, void** iter, GpuVideoDecoderInitDoneParam* r) {
  if (!ReadParam(m, iter, &r->success) ||
      !ReadParam(m, iter, &r->input_buffer_size) ||
      !ReadParam(m, iter, &r->input_buffer_handle))
    return false;
  return true;
}

void ParamTraits<GpuVideoDecoderInitDoneParam>::Log(
    const GpuVideoDecoderInitDoneParam& p, std::string* l) {
  l->append(StringPrintf("(%d %d)", p.success, p.input_buffer_size));
}

///////////////////////////////////////////////////////////////////////////////

void ParamTraits<GpuVideoDecoderInputBufferParam>::Write(
    Message* m, const GpuVideoDecoderInputBufferParam& p) {
  WriteParam(m, p.timestamp);
  WriteParam(m, p.offset);
  WriteParam(m, p.size);
}

bool ParamTraits<GpuVideoDecoderInputBufferParam>::Read(
    const Message* m, void** iter, GpuVideoDecoderInputBufferParam* r) {
  if (!ReadParam(m, iter, &r->timestamp) ||
      !ReadParam(m, iter, &r->offset) ||
      !ReadParam(m, iter, &r->size))
    return false;
  return true;
}

void ParamTraits<GpuVideoDecoderInputBufferParam>::Log(
    const GpuVideoDecoderInputBufferParam& p, std::string* l) {
  l->append(StringPrintf("(%d %d %d)",
                         static_cast<int>(p.timestamp),
                         p.offset, p.size));
}

///////////////////////////////////////////////////////////////////////////////

void ParamTraits<GpuVideoDecoderOutputBufferParam>::Write(
    Message* m, const GpuVideoDecoderOutputBufferParam& p) {
  WriteParam(m, p.frame_id);
  WriteParam(m, p.timestamp);
  WriteParam(m, p.duration);
  WriteParam(m, p.flags);
}

bool ParamTraits<GpuVideoDecoderOutputBufferParam>::Read(
    const Message* m, void** iter, GpuVideoDecoderOutputBufferParam* r) {
  if (!ReadParam(m, iter, &r->frame_id) ||
      !ReadParam(m, iter, &r->timestamp) ||
      !ReadParam(m, iter, &r->duration) ||
      !ReadParam(m, iter, &r->flags))
    return false;
  return true;
}

void ParamTraits<GpuVideoDecoderOutputBufferParam>::Log(
    const GpuVideoDecoderOutputBufferParam& p, std::string* l) {
  l->append(StringPrintf("(%d %d %d %x)",
                         p.frame_id,
                         static_cast<int>(p.timestamp),
                         static_cast<int>(p.duration),
                         p.flags));
}

///////////////////////////////////////////////////////////////////////////////

void ParamTraits<GpuVideoDecoderErrorInfoParam>::Write(
    Message* m, const GpuVideoDecoderErrorInfoParam& p) {
  WriteParam(m, p.error_id);
}

bool ParamTraits<GpuVideoDecoderErrorInfoParam>::Read(
    const Message* m, void** iter, GpuVideoDecoderErrorInfoParam* r) {
  if (!ReadParam(m, iter, &r->error_id))
    return false;
  return true;
}

void ParamTraits<GpuVideoDecoderErrorInfoParam>::Log(
    const GpuVideoDecoderErrorInfoParam& p, std::string* l) {
  l->append(StringPrintf("(%d)", p.error_id));
}

///////////////////////////////////////////////////////////////////////////////

void ParamTraits<GpuVideoDecoderFormatChangeParam>::Write(
    Message* m, const GpuVideoDecoderFormatChangeParam& p) {
  WriteParam(m, p.input_buffer_size);
}

bool ParamTraits<GpuVideoDecoderFormatChangeParam>::Read(
    const Message* m, void** iter, GpuVideoDecoderFormatChangeParam* r) {
  if (!ReadParam(m, iter, &r->input_buffer_size))
    return false;
  return true;
}

void ParamTraits<GpuVideoDecoderFormatChangeParam>::Log(
    const GpuVideoDecoderFormatChangeParam& p, std::string* l) {
  l->append(StringPrintf("%d", p.input_buffer_size));
}

}  // namespace IPC
