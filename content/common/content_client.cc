// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/content_client.h"

#include "base/string_piece.h"

namespace content {

static ContentClient* g_client;

void SetContentClient(ContentClient* client) {
  g_client = client;
}

ContentClient* GetContentClient() {
  return g_client;
}

ContentClient::ContentClient()
    : browser_(NULL), plugin_(NULL), renderer_(NULL), utility_(NULL) {
}

ContentClient::~ContentClient() {
}

bool ContentClient::CanSendWhileSwappedOut(const IPC::Message* msg) {
  return false;
}

bool ContentClient::CanHandleWhileSwappedOut(const IPC::Message& msg) {
  return false;
}

std::string ContentClient::GetUserAgent(bool mimic_windows) const {
  return std::string();
}

string16 ContentClient::GetLocalizedString(int message_id) const {
  return string16();
}

// Return the contents of a resource in a StringPiece given the resource id.
base::StringPiece ContentClient::GetDataResource(int resource_id) const {
  return base::StringPiece();
}

#if defined(OS_WIN)
bool ContentClient::SandboxPlugin(CommandLine* command_line,
                                  sandbox::TargetPolicy* policy) {
  return false;
}
#endif

}  // namespace content
