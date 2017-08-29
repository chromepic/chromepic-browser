/*
 * Copyright (C) 2017 University of Georgia. All rights reserved.
 *
 * This file is subject to the terms and conditions defined at
 * https://raw.githubusercontent.com/chromepic/chromepic-browser/master/LICENSE.txt
 *
 */


#include "content/browser/renderer_host/snapshot/snapshot_handler.h"

#include "base/command_line.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "content/browser/renderer_host/snapshot/screenshot.h"
#include "content/common/input_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "ipc/ipc_sender.h"
#include "net/base/mime_util.h"
#include "third_party/WebKit/Source/platform/WindowsKeyboardCodes.h"

#include "base/process/process_handle.h"
#include "base/threading/platform_thread.h"


using base::FilePath;
using base::File;
using blink::WebInputEvent;
using blink::WebKeyboardEvent;
using blink::WebMouseEvent;
using blink::WebGestureEvent;
using base::Time;

namespace content {

SnapshotHandler::SnapshotHandler(IPC::Sender* sender,
                                 InputRouterClient* client,
                                 int routing_id)
    : sender_(sender), 
      client_(client),
      routing_id_(routing_id),
      next_snapshot_id_(1),
      screenshot_enabled(true),
      dom_snapshot_enabled(true),
      selective_screenshot_enabled(true),
      selective_dom_snapshot_enabled(true),
      last_key_press(-1),
      last_key_press_time(-1),
      key_press_interval(5000000),
      last_mouse_move_time(-1),
      web_contents(0),
      url_id(-1){

   //Select Inputs for optimization
   select_input_set.insert(VK_BACK);
   select_input_set.insert(VK_SPACE);
   select_input_set.insert(VK_TAB);
   select_input_set.insert(VK_RETURN);
   select_input_set.insert(VK_ESCAPE);
   select_input_set.insert(VK_DELETE);
   //

   GenerateSiteID();
   GenerateDirectoryName();
   //logger_ = new Logger(static_cast<const void*>(this));
   logger_ = new Logger();
   const base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
   if (command_line.HasSwitch("disable-screenshots"))
      screenshot_enabled = false;
   else if (command_line.HasSwitch("enable-all-screenshots"))
      selective_screenshot_enabled = false;

   if (command_line.HasSwitch("disable-dom-snapshots"))
      dom_snapshot_enabled = false;
   else if (command_line.HasSwitch("enable-all-dom-snapshots"))
      selective_dom_snapshot_enabled = false;

    if (command_line.HasSwitch("disable-randomized-snapshots")) {
       random_snapshots_enabled = false;
       take_random_snapshot = true;
    }
    else {
      random_snapshots_enabled = true;
      RandomizeSnapshot();
    }

    std::ostringstream ss;
    ss << "SnapshotHandler::SnapshotHandler: Flags- " << "Screenshot Enabled: " << screenshot_enabled << ", Selective Screenshot Enabled: " << selective_screenshot_enabled 
        << ", DOM Snapshot Enabled: " << dom_snapshot_enabled << ", Selective DOM Snapshot Enabled: " << selective_dom_snapshot_enabled << ", Randomization Enabled: " <<
        random_snapshots_enabled << ", Taking Random Snapshot: " << take_random_snapshot;
    logger_->LogLineScreen(ss.str());
}

SnapshotHandler::~SnapshotHandler(){
    fprintf(stderr, "In SnapshotHandler destructor!!");
}

FilePath SnapshotHandler::GetMHTMLFilePath(){
        FilePath cur;

        #if defined(OS_ANDROID)
            PathService::Get(base::DIR_ANDROID_EXTERNAL_STORAGE, &cur);
            cur = cur.Append(FILE_PATH_LITERAL("Download"));
        #else
            PathService::Get(base::DIR_HOME, &cur);
        #endif
        
        cur = cur.Append(FILE_PATH_LITERAL("dom_snapshots"));
        #if defined(OS_POSIX)
            cur = cur.Append(output_directory_name);
        #elif defined(OS_WIN)
            std::wstring name_wstring(output_directory_name.begin(), output_directory_name.end());
            cur = cur.Append(name_wstring);
        #endif

        //TODO(ChromePic): Move this to a file thread and remove the
        //dirty override in base/threading/thread_restrictions.cc
        File::Error error;
        if (!base::CreateDirectoryAndGetError(cur, &error)){
	    std::ostringstream ss;
	    ss << "SnapshotHandler:: GetMHTMLFilePath error in creating directory! ";
	    logger_->LogLineScreen(ss.str(), true);

            fprintf(stderr, "Error in creating an output directory for the snapshots!\n");
            return cur;
        }
        std::string file_name = "snapshot_" + std::to_string(next_snapshot_id_) + ".mhtml";
        //std::string file_name = "snapshot_" + std::to_string(0) + ".html";

        #if defined(OS_POSIX)
            cur = cur.Append(file_name);
        #elif defined(OS_WIN)
            std::wstring wfile_name(file_name.begin(), file_name.end());
            cur = cur.Append(wfile_name);
        #endif
        return cur;
}

void SnapshotHandler::LogEventMetadata(const WebInputEvent *input_event, std::string event_id){
    std::stringstream log_stream;

    if (input_event->type == WebInputEvent::MouseDown || input_event->type == WebInputEvent::MouseUp) {
        const WebMouseEvent& mouse_event = static_cast<const WebMouseEvent&>(*input_event);
        log_stream << "SnapshotHandler:: LogEventMetadata Click Coordinates: " <<  mouse_event.x << ", " << mouse_event.y << ", Event ID: " << event_id;
        Logger::LogLineScreen(log_stream.str(), true);
    }

    if (input_event->type == WebInputEvent::MouseMove) {
        const WebMouseEvent& mouse_event = static_cast<const WebMouseEvent&>(*input_event);
        log_stream << "SnapshotHandler:: LogEventMetadata MouseMove Coordinates: " <<  mouse_event.x << ", " << mouse_event.y << 
                   ", Deltas: " << mouse_event.movementX << ", " << mouse_event.movementY << ", Event ID: " << event_id;
        Logger::LogLineScreen(log_stream.str(), true);
    }

    if (WebInputEvent::isKeyboardEventType(input_event->type)) {
        log_stream << "SnapshotHandler:: LogEventMetadata Key Code: " << static_cast<const WebKeyboardEvent&>(*input_event).windowsKeyCode << ", Event ID: " << event_id;
        Logger::LogLineScreen(log_stream.str(), true);
    }

    // We should pair this with the appropriated TouchStart event during post processing
    if (input_event->type == WebInputEvent::GestureTapDown) {
        const WebGestureEvent& gesture_event = static_cast<const WebGestureEvent&>(*input_event);
        log_stream << "SnapshotHandler:: LogEventMetadata GestureTapDown Coordinates: " <<  gesture_event.x << ", " << gesture_event.y << 
                   ", Event ID: " << event_id;
        Logger::LogLineScreen(log_stream.str(), true);
    }
    log_stream.str("");
}

MHTML_Params SnapshotHandler::GenerateMHTMLParams(bool screenshot_active, bool dom_snapshot_active, std::string event_id){
  MHTML_Params mhtml_params;
  mhtml_params.screenshot_active = screenshot_active;
  mhtml_params.dom_snapshot_active = dom_snapshot_active;
  mhtml_params.snapshot_id = next_snapshot_id_;
  mhtml_params.event_id = event_id;
  if (web_contents && dom_snapshot_active) {
      FilePath file_path = GetMHTMLFilePath();
      uint32_t file_flags = base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE;
      base::File browser_file(file_path, file_flags);
      mhtml_params.destination_file = IPC::GetFileHandleForProcess(
      browser_file.GetPlatformFile(), rvh->GetProcess()->GetHandle(),
      false);  // last parameter: close_file_handle
  }
  mhtml_params.mhtml_boundary_marker = net::GenerateMimeMultipartBoundary(); 
  /*
  InputMsg_HandleInputEvent *msg = new InputMsg_HandleInputEvent(routing_id_, input_event, *latency_info, mhtml_params);
    if (!sender_->Send(msg)) {
    TRACE_EVENT0("forensics", "ERROR->SendAnEventFromBuffer: Failure in sending the event");
   }
  */
  return mhtml_params;
}

void SnapshotHandler::GenerateSiteID(){
    std::stringstream ss;
    const void* site_id_pointer = static_cast<const void*>(this);
    ss << site_id_pointer; 
    site_id = ss.str();
}

void SnapshotHandler::GenerateDirectoryName(){
    Time time_now = Time::Now();
    Time::Exploded exploded_now;
    time_now.LocalExplode(&exploded_now);
    std::stringstream sstream;
    sstream << exploded_now.day_of_month << "_" << exploded_now.month << "_" << exploded_now.year << "__" 
       << exploded_now.hour << "_" << exploded_now.minute << "_" << exploded_now.second <<  "_" << site_id;
    output_directory_name = sstream.str();

    std::ostringstream ss;
    ss << "SnapshotHandler:: Directory generated: " << output_directory_name;
    logger_->LogLineScreen(ss.str(), true);
}

void SnapshotHandler::ScreenshotCaptured(
        int snapshot_id, const SkBitmap& bitmap,
                                        content::ReadbackResponse response) {

    InputEventArg* input_event;
    input_event = FindInputEvent(snapshot_id);
    std::ostringstream ss;
    ss << "Screenshot callback,\t Event ID: " << input_event->event_id;
    logger_->LogLineScreen(ss.str(), true);

    sender_->Send(new InputMsg_ScreenshotCaptured(routing_id_, snapshot_id, input_event->event_id, false)); 
    //fprintf(stderr, "Snapshot_id is: %d \n", snapshot_id);
    if (!input_event) {
        fprintf(stderr, "SnapshotHandler::ScreenshotCaptured: Error, no input event found\n");
        return;
    }

    input_event->screenshot_received = true;
    input_event->UpdateStatus();
    if (response == content::READBACK_SUCCESS) {
         BrowserThread::PostTask(BrowserThread::FILE, FROM_HERE, 
                 base::Bind(&PrintScreenshot, bitmap, output_directory_name, snapshot_id));
    }
    else {
        //fprintf(stderr, "Failed to capture screenshot :( \n");
    }
}

void SnapshotHandler::SendScreenshotRequest(std::string event_id){
      //TODO(ChromePic): The object might not be alive during callback! Change this...

      gfx::Size size = gfx::Size();
      size.process_id = process_id_; 
      size.routing_id = routing_id_;
      size.snapshot_id = next_snapshot_id_;
      size.event_id = event_id;

      std::ostringstream ss;
      ss << "DEBUG Screenshot Request being made,\t Process ID: " << base::GetUniqueIdForProcess() << ", Thread ID: " << base::PlatformThread::CurrentId();
      logger_->LogLineScreen(ss.str(), true);
      
      client_->CopyFromBackingStoreProxy(
                              gfx::Rect(),
                              size,
                              base::Bind(&SnapshotHandler::ScreenshotCaptured, base::Unretained(this), next_snapshot_id_),
                              kN32_SkColorType);
      //}
}

bool SnapshotHandler::RandomizeSnapshot(){
  if (!(rand() % 2))
    take_random_snapshot = false;
  else
    take_random_snapshot = true;
  return take_random_snapshot;
}


void SnapshotHandler::GetRVH(const ui::LatencyInfo& latency_info) {
    int routing_id;
    int process_id;

    web_contents = -1;
    
    std::ostringstream log_stream;

    //TODO(ChromePic): Make this neater!!
    for (const auto& lc : latency_info.latency_components()) {
        routing_id = lc.first.second & 0xffffffff;
        process_id = (lc.first.second >> 32) & 0xffffffff;
        //The 1st component usually has 0,0
        if (process_id != 0)
            break;
    }
    if (process_id != 0) {
        process_id_ = process_id;
        //RenderFrameHostImpl* rfh = RenderFrameHostImpl::FromID(process_id, routing_id);
        rvh = RenderViewHost::FromID(process_id, routing_id);
        if (rvh) {
            log_stream << "RVH address (rvh):" <<  rvh << "routing_id: " << routing_id << ", routing_id_: " << routing_id_; 
            Logger::LogLineScreen(log_stream.str(), true);
            log_stream.str("");
            web_contents = 1;
        }
    }
}




void SnapshotHandler::HandleInputEvent(const WebInputEvent& input_event,
                                      const ui::LatencyInfo& latency_info) {
  std::ostringstream log_stream;
  if(web_contents==0) {
    GetRVH(latency_info);
  }

  bool screenshot_active = screenshot_enabled;
  bool dom_snapshot_active = dom_snapshot_enabled;
  if (WebInputEvent::isKeyboardEventType(input_event.type)) {
      //fprintf(stderr, "Keyboard value: %d\n", static_cast<const WebKeyboardEvent&>(input_event).windowsKeyCode);
      if (select_input_set.find(static_cast<const WebKeyboardEvent&>(input_event).windowsKeyCode) != select_input_set.end()) {
          //fprintf(stderr, "Select Input entered!! \n");
      }
      else {
          // If selective snapshots are enabled, then do not take snapshots for keyboard events
          // that are not among the "select" inputs
          // Except, if the last_key_press was one of select inputs and this key press is
          // new!!
          if (!(select_input_set.find(last_key_press) != select_input_set.end() && last_key_press != static_cast<const WebKeyboardEvent&>(input_event).windowsKeyCode)) {
              if (selective_screenshot_enabled && screenshot_enabled)
                  screenshot_active = false;
              if (selective_dom_snapshot_enabled && dom_snapshot_enabled) 
                  dom_snapshot_active = false;
          }
       }
  }
  //

  bool is_snapshot_event = false;
  if (input_event.type == WebInputEvent::MouseDown ||
      input_event.type == WebInputEvent::RawKeyDown|| input_event.type == WebInputEvent::GestureTapDown)
      is_snapshot_event = true;

  long current_time = base::TimeTicks::Now().ToInternalValue();

  // If Mouse has been moved/"wheeled" for the first time on this page or mouse has not been move for the past
  // `key_press_interval` seconds, then take a snapshot
  if (input_event.type == WebInputEvent::MouseMove || input_event.type == WebInputEvent::MouseWheel) {
      if ((last_mouse_move_time == -1) || ((current_time - last_mouse_move_time) > key_press_interval))
          is_snapshot_event = true;
      last_mouse_move_time = current_time;
  }


//If the key press is same as previous, then, disable snapshotting
  //Note: This happens irrespective of whether it is selective snapshotting or not
  if (input_event.type == WebInputEvent::RawKeyDown) {
      if (static_cast<const WebKeyboardEvent&>(input_event).windowsKeyCode == last_key_press)
          is_snapshot_event = false;
  }

  //If it has been key_press_interval time since last key_press  or if this is the 
  //first key event then, enable snapshotting regardless
  //Note: This also makes sense when all snapshotting is enabled as continuous
  //key strokes ar not usually covered in such cases
  if (input_event.type == WebInputEvent::RawKeyDown) {
     if (((current_time - last_key_press_time) > key_press_interval) ||
         (last_key_press_time == -1)) {
        is_snapshot_event = true;
        if (selective_screenshot_enabled && screenshot_enabled)
            screenshot_active = true;
        if (selective_dom_snapshot_enabled && dom_snapshot_enabled) 
            dom_snapshot_active = true;
     }
     last_key_press = static_cast<const WebKeyboardEvent&>(input_event).windowsKeyCode;
     last_key_press_time = current_time;
  }
  
    
  // When no RVH is found, i.e. there is only a Render Widget, then WebContents is not
  // available and DOM Snapshot cannot be taken. 
  if (web_contents != 1)
      dom_snapshot_active = false;

  if (is_snapshot_event)
  {
      if (web_contents == 1) {
        WebContents* wc = WebContents::FromRenderViewHost(rvh);
        const GURL url = wc->GetLastCommittedURL();
        if (url_id == -1)
            url_id = 1;
        else if(url != last_url){
            if(random_snapshots_enabled)
                log_stream << "SnapshotHandler:: URL Changed. Randomized Snapshot: " << RandomizeSnapshot();
            else 
                log_stream << "SnapshotHandler:: URL Changed";
            Logger::LogLineScreen(log_stream.str(), true);
            log_stream.str("");
            last_key_press = -1;
            last_key_press_time = -1;
            last_mouse_move_time = -1;
            url_id += 1;
        }
        /* This is crashing for certain pages on Android! */
        #if defined(OS_ANDROID)
        #else
        log_stream << "SnapshotHandler:: HandleInputEvent URL: " <<  url.possibly_invalid_spec().c_str()
                   << ", URL ID: " << url_id;
        Logger::LogLineScreen(log_stream.str(), true);
        log_stream.str("");
        #endif
        last_url = url;
     }
     else {
        log_stream << "SnapshotHandler:: No URL obtained!!";
        Logger::LogLineScreen(log_stream.str(), true);
        log_stream.str("");
        }
  }


  std::string event_id;
  log_stream << site_id << "_" << url_id << "_" <<latency_info.trace_id();
  event_id = log_stream.str();
  log_stream.str("");

  log_stream <<"SnapshotHandler:: Event ID: " << event_id << ", Snapshot Page: " << take_random_snapshot; 
  Logger::LogLineScreen(log_stream.str(), true);
  log_stream.str("");

  if (screenshot_active || dom_snapshot_active) {
    if (is_snapshot_event) {
        log_stream << "SnapshotHandler:: This is a snapshot worthy event, Event ID: " << event_id; 
        Logger::LogLineScreen(log_stream.str(), true);
        log_stream.str("");
    }
  }

  if (!take_random_snapshot) {
      screenshot_active = false;
      dom_snapshot_active = false;
  }

  if (screenshot_active || dom_snapshot_active) {
    if (is_snapshot_event) {
        log_stream << "SnapshotHandler:: This is a snapshot event, Event ID: " << event_id << 
        ", Snapshot ID: " << next_snapshot_id_ << ", Output Directory: " << output_directory_name;
        Logger::LogLineScreen(log_stream.str(), true);
        log_stream.str("");
    }
  }

  InputEventArg *input_event_arg = new InputEventArg(input_event, latency_info, 
          is_snapshot_event, screenshot_active, dom_snapshot_active, event_id);
  if(is_snapshot_event && (screenshot_active || dom_snapshot_active)) {
    input_event_arg->SetSnapshotID(next_snapshot_id_);
  log_stream << "SnapshotHandler:: Putting into input event buffer Event ID: " << (*input_event_arg).event_id << 
                ", Snapshot ID: " << (*input_event_arg).snapshot_id; 
  Logger::LogLineScreen(log_stream.str(), true);
  log_stream.str("");
  input_event_buffer.push_front(*input_event_arg);
  }

  MHTML_Params mhtml_params = GenerateMHTMLParams(is_snapshot_event && screenshot_active, is_snapshot_event && dom_snapshot_active,
                                                  event_id);
  if (is_snapshot_event) {
    if (screenshot_active) {
        SendScreenshotRequest(event_id);
    }
    if (screenshot_active || dom_snapshot_active)
        next_snapshot_id_ ++;
 }
 InputMsg_HandleInputEvent *msg = new InputMsg_HandleInputEvent(routing_id_, &input_event, latency_info, mhtml_params);
 if (!sender_->Send(msg)) {
    TRACE_EVENT0("forensics", "ERROR->SendAnEventFromBuffer: Failure in sending the event");
 }
 LogEventMetadata(&input_event, event_id);
}

//Note: This is not thread-safe!
InputEventArg* SnapshotHandler::FindInputEvent(int snapshot_id) {
    std::ostringstream log_stream;
    for (std::deque<InputEventArg>::reverse_iterator it = input_event_buffer.rbegin(); it!=input_event_buffer.rend(); it++) {
        if (!(*it).is_snapshot_event)
            continue;
        //fprintf(stderr, "Snapshot_id found is: %d \n", (*it).snapshot_id);
        if ((*it).snapshot_id == snapshot_id) {
            log_stream <<"SnapshotHandler::FindInputEvent Event ID: " << (*it).event_id << ", Snapshot ID: " << (*it).snapshot_id << ", Looking for: " << (*it).snapshot_id; 
            Logger::LogLineScreen(log_stream.str(), true);
            log_stream.str("");
            return &(*it);
        }
    }
    fprintf(stderr, "NO Snapshot_id found! \n");
    return NULL;
}
}
