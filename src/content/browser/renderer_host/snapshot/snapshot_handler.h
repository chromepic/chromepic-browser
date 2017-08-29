/*
 * Copyright (C) 2017 University of Georgia. All rights reserved.
 *
 * This file is subject to the terms and conditions defined at
 * https://raw.githubusercontent.com/chromepic/chromepic-browser/master/LICENSE.txt
 *
 */


//#include <queue>

#include <set>
#include "content/browser/renderer_host/input/input_router_client.h"
#include "content/browser/renderer_host/snapshot/input_event_arg.h"
#include "content/browser/renderer_host/snapshot/logger.h"
#include "content/public/browser/readback_types.h"
#include "content/public/browser/render_view_host.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"
#include "ui/events/latency_info.h"
#include "url/gurl.h"

struct MHTML_Params;

namespace IPC {
    class Sender;
}

namespace content {

class SnapshotHandler {
 public:
  SnapshotHandler(IPC::Sender* sender,
                 InputRouterClient* client,
                 int routing_id);
  ~SnapshotHandler();
  base::FilePath GetMHTMLFilePath();
  void ScreenshotCaptured(
          int snapshot_id,
          const SkBitmap& bitmap,
          content::ReadbackResponse response);
  void DOMSnapshotCaptured(
          int snapshot_id,
          int64_t size);
  void SendScreenshotRequest(std::string event_id);
  void LogEventMetadata(const blink::WebInputEvent *input_event, std::string event_id);
  void HandleInputEvent(const blink::WebInputEvent& input_event,
                        const ui::LatencyInfo& latency_info);
  void GetRVH(const ui::LatencyInfo& latency_info);
  MHTML_Params GenerateMHTMLParams(bool screenshot_active, bool dom_snapshot_active, std::string event_id);
  InputEventArg* FindInputEvent(int snapshot_id);
  bool RandomizeSnapshot();
  Logger* logger_;

 private:
  std::string site_id;
  void GenerateSiteID();
  void GenerateDirectoryName();
  IPC::Sender* sender_;
  InputRouterClient* client_;
  int process_id_;
  int routing_id_;
  int next_snapshot_id_;
  bool screenshot_enabled;
  bool dom_snapshot_enabled; 
//Enable snapshot for selective inputs
  bool selective_screenshot_enabled;
  bool selective_dom_snapshot_enabled; 
  std::set<int> select_input_set;
  std::string output_directory_name;
  std::deque<InputEventArg> input_event_buffer;
  // Queue containing snapshot IDs for pending (and in process) snapshot events
  std::deque<int> pending_snapshots_queue;
  // Store the last key press code
  int last_key_press;
  long last_key_press_time;
  long key_press_interval;
  // Stores when the last mouse move happened
  long last_mouse_move_time;

//Random Snapshot Options for experimental evaluation
  bool random_snapshots_enabled;
  bool take_random_snapshot;

  // 0 : not set, 1 : found, -1 : not found
  int web_contents;
  RenderViewHost* rvh;

  // Last url
 GURL last_url;
 int url_id;
};

}
