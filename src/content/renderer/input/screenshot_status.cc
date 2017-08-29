/*
 * Copyright (C) 2017 University of Georgia. All rights reserved.
 *
 * This file is subject to the terms and conditions defined at
 * https://raw.githubusercontent.com/chromepic/chromepic-browser/master/LICENSE.txt
 *
 */


#include "base/memory/singleton.h"
#include "content/renderer/input/screenshot_status.h"

ScreenshotStatus* ScreenshotStatus::GetInstance() {
    return base::Singleton<ScreenshotStatus>::get();
}

ScreenshotStatus::ScreenshotStatus() {
}

ScreenshotStatus::~ScreenshotStatus() {
    captured_screenshots.clear();
}
