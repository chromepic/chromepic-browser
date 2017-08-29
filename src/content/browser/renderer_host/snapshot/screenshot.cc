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


#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "content/browser/renderer_host/snapshot/screenshot.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"

using content::BrowserThread;
using base::FilePath;
using base::File;
using base::Time;

void PrintScreenshot(const SkBitmap& bitmap, std::string output_directory_name, const int snapshot_id) {
        TRACE_EVENT_BEGIN1("forensics", "PrintScreenshot: Begin", "snapshot ID", snapshot_id);
        FilePath cur;
        #if defined(OS_ANDROID)
            PathService::Get(base::DIR_ANDROID_EXTERNAL_STORAGE, &cur);
            cur = cur.Append(FILE_PATH_LITERAL("Download"));
        #else
            PathService::Get(base::DIR_HOME, &cur);
        #endif

        cur = cur.Append(FILE_PATH_LITERAL("snapshots"));

        #if defined(OS_POSIX)
            cur = cur.Append(output_directory_name);
        #elif defined(OS_WIN)
            std::wstring name_wstring(output_directory_name.begin(), output_directory_name.end());
            cur = cur.Append(name_wstring);
        #endif

        File::Error error;
        if (!base::CreateDirectoryAndGetError(cur, &error)){
            fprintf(stderr, "Error in creating an output directory for the snapshots!\n");
            return;
        }
        std::string file_name = "snapshot_" + std::to_string(snapshot_id) + ".png";

        #if defined(OS_POSIX)
            cur = cur.Append(file_name);
        #elif defined(OS_WIN)
            std::wstring wfile_name(file_name.begin(), file_name.end());
            cur = cur.Append(wfile_name);
        #endif

        //fprintf(stderr, "Captured screenshot of size: %lu!! \n", bitmap.getSize());
        std::vector<unsigned char> png_data;
        bool res = gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false, &png_data);
        //fprintf(stderr, "PNG Encode attempted.. Result: %d\n", res);
        if (!res) 
            return;
        ////fprintf(stderr, "Writing to file: %s \n", cur.value().c_str());
        base::WriteFile(cur, reinterpret_cast<const char*>(&png_data[0]), png_data.size());
        //fprintf(stderr, "Wrote file to disk.\n");
        TRACE_EVENT_END1("forensics", "PrintScreenshot: End", "snapshot ID", snapshot_id);
}
