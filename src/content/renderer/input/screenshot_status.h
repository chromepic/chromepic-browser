/*
 * Copyright (C) 2017 University of Georgia. All rights reserved.
 *
 * This file is subject to the terms and conditions defined at
 * https://raw.githubusercontent.com/chromepic/chromepic-browser/master/LICENSE.txt
 *
 */


#include <unordered_set>
#include "base/synchronization/lock.h"

template <typename T> struct DefaultSingletonTraits;
class ScreenshotStatus {
   public:
    static ScreenshotStatus* GetInstance(); 
    std::unordered_set<int> captured_screenshots;
    ScreenshotStatus(); 
    ~ScreenshotStatus(); 
    base::Lock ss_lock;
   private:
    friend struct DefaultSingletonTraits<ScreenshotStatus>;

    DISALLOW_COPY_AND_ASSIGN(ScreenshotStatus);
  };
