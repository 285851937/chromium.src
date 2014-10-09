// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Needed on Windows to get |M_PI| from <cmath>.
#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

#include <cmath>

#include "content/browser/renderer_host/input/web_input_event_util.h"
#include "content/common/input/web_input_event_traits.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event_constants.h"
#include "ui/events/gesture_detection/motion_event_generic.h"

using blink::WebInputEvent;
using blink::WebTouchEvent;
using blink::WebTouchPoint;
using ui::MotionEvent;
using ui::MotionEventGeneric;

namespace content {

TEST(WebInputEventUtilTest, MotionEventConversion) {
  ui::PointerProperties pointer(5, 10);
  pointer.id = 15;
  pointer.raw_x = 20;
  pointer.raw_y = 25;
  pointer.pressure = 30;
  pointer.touch_minor = 35;
  pointer.touch_major = 40;
  pointer.orientation = static_cast<float>(-M_PI / 2);
  MotionEventGeneric event(
      MotionEvent::ACTION_DOWN, base::TimeTicks::Now(), pointer);
  event.set_flags(ui::EF_SHIFT_DOWN | ui::EF_ALT_DOWN);

  WebTouchEvent expected_event;
  expected_event.type = WebInputEvent::TouchStart;
  expected_event.touchesLength = 1;
  expected_event.timeStampSeconds =
      (event.GetEventTime() - base::TimeTicks()).InSecondsF();
  expected_event.modifiers = WebInputEvent::ShiftKey | WebInputEvent::AltKey;
  WebTouchPoint expected_pointer;
  expected_pointer.id = pointer.id;
  expected_pointer.state = WebTouchPoint::StatePressed;
  expected_pointer.position = blink::WebFloatPoint(pointer.x, pointer.y);
  expected_pointer.screenPosition =
      blink::WebFloatPoint(pointer.raw_x, pointer.raw_y);
  expected_pointer.radiusX = pointer.touch_major / 2.f;
  expected_pointer.radiusY = pointer.touch_minor / 2.f;
  expected_pointer.rotationAngle = 0.f;
  expected_pointer.force = pointer.pressure;
  expected_event.touches[0] = expected_pointer;

  WebTouchEvent actual_event = CreateWebTouchEventFromMotionEvent(event);
  EXPECT_EQ(WebInputEventTraits::ToString(expected_event),
            WebInputEventTraits::ToString(actual_event));
}

}  // namespace content
