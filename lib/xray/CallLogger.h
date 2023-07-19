//
// Created by sebastian on 14.07.23.
//

#ifndef CAPI_CALLLOGGER_H
#define CAPI_CALLLOGGER_H

#include <string>

#include "XRayRuntime.h"

namespace capi {

class CallLogger {
  std::string basename;
  std::string logfile;
  std::vector<std::string> buffer;
  const size_t bufferCapacity{1000};
  bool fileIsSet{false};
public:
  explicit CallLogger(std::string_view basename) XRAY_NEVER_INSTRUMENT : basename(basename) {
    buffer.reserve(bufferCapacity);

  }

  ~CallLogger() XRAY_NEVER_INSTRUMENT {
    flush();
  }

  void logEnter(int depth, const XRayFunctionInfo& info) XRAY_NEVER_INSTRUMENT;
  void logExit(int depth, const XRayFunctionInfo& info) XRAY_NEVER_INSTRUMENT;
  void makeLogEntry(int depth, std::string_view type,
                           const capi::XRayFunctionInfo &info) XRAY_NEVER_INSTRUMENT;

  void determineLogfile() XRAY_NEVER_INSTRUMENT;

  void flush() XRAY_NEVER_INSTRUMENT;
};

}

#endif // CAPI_CALLLOGGER_H
