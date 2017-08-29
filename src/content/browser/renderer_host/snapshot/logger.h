/*
 * Copyright (C) 2017 University of Georgia. All rights reserved.
 *
 * This file is subject to the terms and conditions defined at
 * https://raw.githubusercontent.com/chromepic/chromepic-browser/master/LICENSE.txt
 *
 */


#include "base/files/file_path.h"

namespace content {

class Logger {
 public:
  Logger(std::string name="");
  ~Logger();
  static void LogLineScreen(std::string log_string, bool add_time=false);
  void LogLine(std::string log_stream, bool add_time=false, bool flush=false);
  void Flush();

  std::stringstream log_stream;

 private:
  base::FilePath file_path;

};

}
