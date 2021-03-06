// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_synchronous_compositor_android.h"

#include "cc/output/compositor_frame.h"

namespace content {

TestSynchronousCompositor::TestSynchronousCompositor()
    : client_(NULL), hardware_initialized_(false) {
}

TestSynchronousCompositor::~TestSynchronousCompositor() {
  DCHECK(!hardware_initialized_);
  SetClient(NULL);
}

void TestSynchronousCompositor::SetClient(SynchronousCompositorClient* client) {
  if (client_)
    client_->DidDestroyCompositor(this);
  client_ = client;
  if (client_)
    client_->DidInitializeCompositor(this);
}

bool TestSynchronousCompositor::InitializeHwDraw() {
  DCHECK(!hardware_initialized_);
  hardware_initialized_ = true;
  return true;
}

void TestSynchronousCompositor::ReleaseHwDraw() {
  DCHECK(hardware_initialized_);
  hardware_initialized_ = false;
}

scoped_ptr<cc::CompositorFrame> TestSynchronousCompositor::DemandDrawHw(
    gfx::Size surface_size,
    const gfx::Transform& transform,
    gfx::Rect viewport,
    gfx::Rect clip,
    gfx::Rect viewport_rect_for_tile_priority,
    const gfx::Transform& transform_for_tile_priority) {
  DCHECK(hardware_initialized_);
  return nullptr;
}

void TestSynchronousCompositor::ReturnResources(
    const cc::CompositorFrameAck& frame_ack) {
  DCHECK(hardware_initialized_);
}

bool TestSynchronousCompositor::DemandDrawSw(SkCanvas* canvas) {
  DCHECK(canvas);
  return true;
}

void TestSynchronousCompositor::SetMemoryPolicy(size_t bytes_limit) {
  DCHECK(!bytes_limit || hardware_initialized_) << bytes_limit;
}

}  // namespace content
