/*
 * Copyright (C) 2017 University of Georgia. All rights reserved.
 *
 * This file is subject to the terms and conditions defined at
 * https://raw.githubusercontent.com/chromepic/chromepic-browser/master/LICENSE.txt
 *
 */


#include <stdlib.h>
#include <stdio.h>

#include "content/browser/renderer_host/snapshot/input_event_arg.h"


using blink::WebGestureEvent;
using blink::WebInputEvent;
using blink::WebKeyboardEvent;
using blink::WebMouseEvent;
using blink::WebMouseWheelEvent;
using blink::WebTouchEvent;
using blink::WebTouchPoint;

namespace content {

InputEventArg::InputEventArg(const WebInputEvent& input_event_src, const ui::LatencyInfo& latency_info,
            bool is_snapshot_event, bool screenshot_enabled, bool dom_snapshot_enabled, std::string event_id):
            is_snapshot_event(is_snapshot_event),
            screenshot_enabled(screenshot_enabled),
            screenshot_received(false),
            dom_snapshot_enabled(dom_snapshot_enabled),
            dom_snapshot_received(false),
            event_id(event_id) {
    InputEventArg::CopyInputEvent(input_event_src);
    InputEventArg::CopyLatencyInfo(latency_info);
    if (is_snapshot_event)
      status = Wait;
    else
      status = Ready;
    InputEventArg::UpdateStatus();
    //fprintf(stderr, "InputEventArg::Created InputEventArg\n");
}

void InputEventArg::SetSnapshotID(int snapshot_id_param) {
    snapshot_id = snapshot_id_param;
}

void InputEventArg::UpdateStatus() {
  if ( is_snapshot_event &&
      ((screenshot_received && screenshot_enabled) || (!screenshot_enabled)) &&
      ((dom_snapshot_received && dom_snapshot_enabled) || (!dom_snapshot_enabled)))
    status = Ready;
}

void InputEventArg::CopyInputEvent(const WebInputEvent& input_event_src) {
    void* event_copy;
    WebInputEvent::Type type = input_event_src.type;
    if (WebInputEvent::isMouseEventType(type)) {
      //fprintf(stderr, "InputEventArg: Mouse type\n");
      event_copy = malloc(sizeof(WebMouseEvent));
      memcpy(event_copy, &input_event_src, sizeof(WebMouseEvent));
      input_event = static_cast<WebInputEvent*>(event_copy);
    }
    else if (type == WebInputEvent::MouseWheel){
      //fprintf(stderr, "InputEventArg: MouseWheel type\n");
      event_copy = malloc(sizeof(WebMouseWheelEvent));
      memcpy(event_copy, &input_event_src, sizeof(WebMouseWheelEvent));
      input_event = static_cast<WebInputEvent*>(event_copy);
    }
    else if (WebInputEvent::isKeyboardEventType(type)) {
      //fprintf(stderr, "InputEventArg: Keyboard type\n");
      event_copy = malloc(sizeof(WebKeyboardEvent));
      memcpy(event_copy, &input_event_src, sizeof(WebKeyboardEvent));
      input_event = static_cast<WebInputEvent*>(event_copy);
    }
    else if (WebInputEvent::isTouchEventType(type)) {
      //fprintf(stderr, "InputEventArg: Touch type\n");
      event_copy = malloc(sizeof(WebTouchEvent));
      memcpy(event_copy, &input_event_src, sizeof(WebTouchEvent));
      input_event = static_cast<WebInputEvent*>(event_copy);
    }
    else if (WebInputEvent::isGestureEventType(type)) {
      //fprintf(stderr, "InputEventArg: Gesture type\n");
      event_copy = malloc(sizeof(WebGestureEvent));
      memcpy(event_copy, &input_event_src, sizeof(WebGestureEvent));
      input_event = static_cast<WebInputEvent*>(event_copy);
    }
}

void InputEventArg::CopyLatencyInfo(const ui::LatencyInfo& latency_info_src) {
        void* copy;
        copy = malloc(sizeof(ui::LatencyInfo));
        memcpy(copy, &latency_info_src, sizeof(ui::LatencyInfo));
        latency_info = static_cast<ui::LatencyInfo*>(copy);
}

InputEventArg::~InputEventArg(){
    //fprintf(stderr, "InputEventArg: In the destructor bruh!\n");
    free(input_event);
    free(latency_info);
}
}
