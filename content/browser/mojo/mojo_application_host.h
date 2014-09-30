// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MOJO_MOJO_APPLICATION_HOST_H_
#define CONTENT_BROWSER_MOJO_MOJO_APPLICATION_HOST_H_

#include "base/process/process_handle.h"
#include "content/common/mojo/service_registry_impl.h"
#include "mojo/embedder/channel_init.h"
#include "mojo/embedder/scoped_platform_handle.h"

namespace IPC {
class Sender;
}

namespace content {

// MojoApplicationHost represents the code needed on the browser side to setup
// a child process as a Mojo application via Chrome IPC. The child process
// should use MojoApplication to handle messages generated by an instance of
// MojoApplicationHost. MojoApplicationHost makes the ServiceRegistry interface
// available so that child-provided services can be invoked.
class MojoApplicationHost {
 public:
  MojoApplicationHost();
  virtual ~MojoApplicationHost();

  // Two-phase initialization:
  //  1- Init makes service_registry() available synchronously.
  //  2- Activate establishes the actual connection to the peer process.
  bool Init();
  void Activate(IPC::Sender* sender, base::ProcessHandle process_handle);

  void WillDestroySoon();

  ServiceRegistry* service_registry() { return &service_registry_; }

 private:
  mojo::embedder::ChannelInit channel_init_;
  mojo::embedder::ScopedPlatformHandle client_handle_;

  bool did_activate_;

  ServiceRegistryImpl service_registry_;

  DISALLOW_COPY_AND_ASSIGN(MojoApplicationHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MOJO_MOJO_APPLICATION_HOST_H_
