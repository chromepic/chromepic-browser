/*
 * Copyright (C) 2017 University of Georgia. All rights reserved.
 *
 * This file is subject to the terms and conditions defined at
 * https://raw.githubusercontent.com/chromepic/chromepic-browser/master/LICENSE.txt
 *
 */


#include "content/public/browser/readback_types.h"

//void ScreenshotCaptured(int snapshot_id, const SkBitmap& bitmap, content::ReadbackResponse response);
void PrintScreenshot(const SkBitmap& bitmap, std::string tab_id, const int screenshot_id);
