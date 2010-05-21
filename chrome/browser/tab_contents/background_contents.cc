// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/tab_contents/background_contents.h"

#include "chrome/browser/browser.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/browsing_instance.h"
#include "chrome/browser/in_process_webkit/dom_storage_context.h"
#include "chrome/browser/in_process_webkit/webkit_context.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/renderer_host/render_view_host.h"
#include "chrome/browser/renderer_host/site_instance.h"
#include "chrome/browser/renderer_preferences_util.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/view_types.h"
#include "chrome/common/render_messages.h"


////////////////
// BackgroundContents

BackgroundContents::BackgroundContents(SiteInstance* site_instance,
                                       int routing_id) {
  Profile* profile = site_instance->browsing_instance()->profile();

  // TODO(rafaelw): Implement correct session storage.
  int64 session_storage_namespace_id = profile->GetWebKitContext()->
      dom_storage_context()->AllocateSessionStorageNamespaceId();
  render_view_host_ = new RenderViewHost(site_instance, this, routing_id,
                                         session_storage_namespace_id);
  render_view_host_->AllowScriptToClose(true);

#if defined(OS_WIN) || defined(OS_LINUX)
  registrar_.Add(this, NotificationType::BROWSER_CLOSED,
                 NotificationService::AllSources());
#elif defined(OS_MACOSX)
  registrar_.Add(this, NotificationType::APP_TERMINATING,
                 NotificationService::AllSources());
#endif
}

void BackgroundContents::Observe(NotificationType type,
                                 const NotificationSource& source,
                                 const NotificationDetails& details) {
  // TODO(rafaelw): Implement pagegroup ref-counting so that non-persistent
  // background pages are closed when the last referencing frame is closed.
  switch (type.value) {
#if defined(OS_WIN) || defined(OS_LINUX)
    case NotificationType::BROWSER_CLOSED: {
      bool app_closing = *Details<bool>(details).ptr();
      if (app_closing)
        delete this;
      break;
    }
#elif defined(OS_MACOSX)
    case NotificationType::APP_TERMINATING: {
      delete this;
      break;
    }
#endif
    default:
      NOTREACHED() << "Unexpected notification sent.";
      break;
  }
}

BackgroundContents::~BackgroundContents() {
  NotificationService::current()->Notify(
      NotificationType::BACKGROUND_CONTENTS_DELETED,
      Source<BackgroundContents>(this),
      Details<RenderViewHost>(render_view_host_));
  render_view_host_->Shutdown();  // deletes render_view_host
}

void BackgroundContents::DidNavigate(
    RenderViewHost* render_view_host,
    const ViewHostMsg_FrameNavigate_Params& params) {
  // We only care when the outer frame changes.
  if (!PageTransition::IsMainFrame(params.transition))
    return;

  // Note: because BackgroundContents are only available to extension apps,
  // navigation is limited to urls within the app's extent. This is enforced in
  // RenderView::decidePolicyForNaviation. If BackgroundContents become
  // available as a part of the web platform, it probably makes sense to have
  // some way to scope navigation of a background page to its opener's security
  // origin. Note: if the first navigation is to a URL outside the app's
  // extent a background page will be opened but will remain at about:blank.
  url_ = params.url;

  NotificationService::current()->Notify(
      NotificationType::BACKGROUND_CONTENTS_NAVIGATED,
      Source<BackgroundContents>(this),
      Details<RenderViewHost>(render_view_host_));
}

void BackgroundContents::RunJavaScriptMessage(
    const std::wstring& message,
    const std::wstring& default_prompt,
    const GURL& frame_url,
    const int flags,
    IPC::Message* reply_msg,
    bool* did_suppress_message) {
  // TODO(rafaelw): Implement, The JavaScriptModalDialog needs to learn about
  // BackgroundContents.
  *did_suppress_message = true;
}

std::wstring BackgroundContents::GetMessageBoxTitle(const GURL& frame_url,
                                                    bool is_alert) {
  NOTIMPLEMENTED();
  return L"";
}

gfx::NativeWindow BackgroundContents::GetMessageBoxRootWindow() {
  NOTIMPLEMENTED();
  return NULL;
}

void BackgroundContents::OnMessageBoxClosed(IPC::Message* reply_msg,
                                            bool success,
                                            const std::wstring& prompt) {
  render_view_host_->JavaScriptMessageBoxClosed(reply_msg, success, prompt);
}

void BackgroundContents::Close(RenderViewHost* render_view_host) {
  delete this;
}

RendererPreferences BackgroundContents::GetRendererPrefs(
    Profile* profile) const {
  RendererPreferences preferences;
  renderer_preferences_util::UpdateFromSystemSettings(&preferences, profile);
  return preferences;
}

WebPreferences BackgroundContents::GetWebkitPrefs() {
  // TODO(rafaelw): Consider enabling the webkit_prefs.dom_paste_enabled for
  // apps.
  Profile* profile = render_view_host_->process()->profile();
  return RenderViewHostDelegateHelper::GetWebkitPrefs(profile,
                                                      false);  // is_dom_ui
}

void BackgroundContents::ProcessDOMUIMessage(const std::string& message,
                                             const ListValue* content,
                                             const GURL& source_url,
                                             int request_id,
                                             bool has_callback) {
  // TODO(rafaelw): It may make sense for extensions to be able to open
  // BackgroundContents to chrome-extension://<id> pages. Consider implementing.
  render_view_host_->BlockExtensionRequest(request_id);
}

void BackgroundContents::CreateNewWindow(
    int route_id,
    WindowContainerType window_container_type) {
  delegate_view_helper_.CreateNewWindow(route_id,
                                        render_view_host_->process()->profile(),
                                        render_view_host_->site_instance(),
                                        DOMUIFactory::GetDOMUIType(url_),
                                        this,
                                        window_container_type);
}

void BackgroundContents::CreateNewWidget(int route_id,
                                         WebKit::WebPopupType popup_type) {
  NOTREACHED();
}

void BackgroundContents::ShowCreatedWindow(int route_id,
                                           WindowOpenDisposition disposition,
                                           const gfx::Rect& initial_pos,
                                           bool user_gesture) {
  TabContents* contents = delegate_view_helper_.GetCreatedWindow(route_id);
  if (!contents)
    return;
  Browser* browser = BrowserList::GetLastActiveWithProfile(
      render_view_host_->process()->profile());
  if (!browser)
    return;

  browser->AddTabContents(contents, disposition, initial_pos, user_gesture);
}

void BackgroundContents::ShowCreatedWidget(int route_id,
                                           const gfx::Rect& initial_pos) {
  NOTIMPLEMENTED();
}

