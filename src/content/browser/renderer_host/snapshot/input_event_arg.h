/*
 * Copyright (C) 2017 University of Georgia. All rights reserved.
 *
 * This file is subject to the terms and conditions defined at
 * https://raw.githubusercontent.com/chromepic/chromepic-browser/master/LICENSE.txt
 *
 */


#include "third_party/WebKit/public/web/WebInputEvent.h"
#include "ui/events/latency_info.h"

namespace content {

struct InputEventArg{
   blink::WebInputEvent* input_event;
   ui::LatencyInfo* latency_info;

   enum Status {Ready, Wait};
   Status status;

   bool is_snapshot_event;
   bool screenshot_enabled;
   bool screenshot_received;
   bool dom_snapshot_enabled; 
   bool dom_snapshot_received;
   int snapshot_id;
   std::string event_id;

   InputEventArg(const blink::WebInputEvent& input_event, const ui::LatencyInfo& latency_info,
       bool is_snapshot_event, bool screenshot_enabled, bool dom_snapshot_enabled, std::string event_id);
   ~InputEventArg();
   void SetSnapshotID(int snapshot_id);
   void UpdateStatus();
   void CopyInputEvent(const blink::WebInputEvent& input_event_src);
   void CopyLatencyInfo(const ui::LatencyInfo& latency_info_src);
};

}
