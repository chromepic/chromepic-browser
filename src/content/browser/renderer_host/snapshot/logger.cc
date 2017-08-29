/*
 * Copyright (C) 2017 University of Georgia. All rights reserved.
 *
 * This file is subject to the terms and conditions defined at
 * https://raw.githubusercontent.com/chromepic/chromepic-browser/master/LICENSE.txt
 *
 */


#include <stdio.h>
#include <string>
#include <sstream>

#include "content/browser/renderer_host/snapshot/logger.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "content/public/browser/browser_thread.h"
#if defined(OS_ANDROID)
#include <android/log.h>
#endif

using content::BrowserThread;
using base::FilePath;
using base::File;
using base::TimeTicks;
using base::Time;

namespace content {

Logger::Logger(std::string name) {

        #if defined(OS_ANDROID)
            PathService::Get(base::DIR_ANDROID_EXTERNAL_STORAGE,&file_path);
            file_path = file_path.Append(FILE_PATH_LITERAL("Download"));
        #else
            PathService::Get(base::DIR_HOME, &file_path);
        #endif

        file_path = file_path.Append(FILE_PATH_LITERAL("snapshot_logs"));
        File::Error error;
        if (!base::CreateDirectoryAndGetError(file_path, &error)){
            fprintf(stderr, "Error in creating an output directory for snapshot logs!\n");
            return;
        }
        //std::stringstream ss;
        //ss << tab_id; 
        //std::string tab_id_string = ss.str() + suffix + ".txt";
        if (name.empty()) {
            Time time_now = Time::Now();
            Time::Exploded exploded_now;
            time_now.LocalExplode(&exploded_now);
            std::stringstream ss;
            ss << exploded_now.month << "_" << exploded_now.day_of_month << "_" << exploded_now.year << "__" 
               << exploded_now.hour << "_" << exploded_now.minute << "_" << exploded_now.second << ".txt";
            name = ss.str();
        }

        #if defined(OS_POSIX)
            file_path = file_path.Append(name);
        #elif defined(OS_WIN)
            std::wstring name_wstring(name.begin(), name.end());
            file_path = file_path.Append(name_wstring);
        #endif
        //fprintf(stderr, "Created logger!! %s\n", name.c_str());
}

Logger::~Logger(){
        fprintf(stderr, "Killing logger!! %s\n", file_path.value().c_str());
}

void Logger::LogLine(std::string log_string, bool add_time, bool flush){
    if (add_time){
        int64_t us = TimeTicks::Now().ToInternalValue();
        log_stream << log_string << ",\tTime: " << us << "\n";
    }
    else
        log_stream << log_string << "\n"; 

    if (flush){
    const std::string tmp = log_stream.str();
    const char* cstr = tmp.c_str();
    int ret = base::WriteFile(file_path, cstr, strlen(cstr));
    if (ret < 0)
        fprintf(stderr, "Failure in writing to file!!\n");
    }
}

void Logger::LogLineScreen(std::string log_string, bool add_time){
    std::stringstream log_stream_screen;
    if (add_time){
        int64_t us = TimeTicks::Now().ToInternalValue();
        log_stream_screen << log_string << ",\tTime: " << us << "\n";
    }
    else
        log_stream_screen << log_string << "\n"; 
    fprintf(stderr, "%s", log_stream_screen.str().c_str());
    #if defined(OS_ANDROID)
      va_list args;
      __android_log_vprint(ANDROID_LOG_WARN, "Forensics", log_stream_screen.str().c_str(),args);
    #endif

 }

void Logger::Flush(){
    if (!log_stream.str().size())
        return;

    bool high_res = TimeTicks::IsHighResolution();
    if (high_res)
        LogLine("High resolution clock was available");
    else
        LogLine("High resolution clock was not available!");

    const std::string tmp = log_stream.str();
    const char* cstr = tmp.c_str();
    int ret = base::WriteFile(file_path, cstr, strlen(cstr));
    if (ret < 0)
        fprintf(stderr, "Failure in writing to file!!\n");
    fprintf(stderr, "done flushing for file:%s\n", file_path.value().c_str());
}
}  //namespace content
