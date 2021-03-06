// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/testing_io_thread_state.h"

#include "base/message_loop/message_loop_proxy.h"
#include "base/run_loop.h"
#include "base/time/tick_clock.h"
#include "chrome/browser/io_thread.h"
#include "chrome/test/base/testing_browser_process.h"
#include "content/public/browser/browser_thread.h"

#if defined(OS_CHROMEOS)
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/network/network_handler.h"
#endif

using content::BrowserThread;

namespace {

base::Closure ThreadSafeQuit(base::RunLoop* run_loop) {
  if (BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    return run_loop->QuitClosure();
  } else {
    using base::Bind;
    using base::IgnoreResult;
    return Bind(IgnoreResult(&base::MessageLoopProxy::PostTask),
                base::MessageLoopProxy::current(),
                FROM_HERE,
                run_loop->QuitClosure());
  }
}

}  // namespace

namespace chrome {

TestingIOThreadState::TestingIOThreadState() {
#if defined(OS_CHROMEOS)
  // Needed by IOThread constructor.
  chromeos::DBusThreadManager::Initialize();
  chromeos::NetworkHandler::Initialize();
#endif

  io_thread_state_.reset(
      new IOThread(TestingBrowserProcess::GetGlobal()->local_state(),
                   TestingBrowserProcess::GetGlobal()->policy_service(),
                   NULL, NULL));

  // Safe because there are no virtuals.
  base::RunLoop run_loop;
  CHECK(BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                                base::Bind(&TestingIOThreadState::Initialize,
                                           base::Unretained(this),
                                           ThreadSafeQuit(&run_loop))));
  run_loop.Run();

  TestingBrowserProcess::GetGlobal()->SetIOThread(io_thread_state_.get());
}

TestingIOThreadState::~TestingIOThreadState() {
  // Remove all the local IOThread state.
  base::RunLoop run_loop;
  CHECK(BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                                base::Bind(&TestingIOThreadState::Shutdown,
                                           base::Unretained(this),
                                           ThreadSafeQuit(&run_loop))));
  run_loop.Run();
  TestingBrowserProcess::GetGlobal()->SetIOThread(NULL);

  io_thread_state_.reset();

#if defined(OS_CHROMEOS)
  chromeos::NetworkHandler::Shutdown();
  chromeos::DBusThreadManager::Shutdown();
#endif
}

void TestingIOThreadState::Initialize(const base::Closure& done) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  io_thread_state_->SetGlobalsForTesting(new IOThread::Globals());

  done.Run();
}

void TestingIOThreadState::Shutdown(const base::Closure& done) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  delete io_thread_state_->globals();
  io_thread_state_->SetGlobalsForTesting(NULL);
  done.Run();
}

}  // namespace chrome
