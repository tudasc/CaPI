//
// Created by sebastian on 21.08.23.
//

#ifndef CAPI_LOGGING_H
#define CAPI_LOGGING_H

#include <iostream>

namespace capi {

enum LogLevel { LOG_NONE, LOG_CRITICAL, LOG_STATUS, LOG_EXTRA };

inline std::ostream &logInfo() { return std::cout << "[Info] "; }

inline std::ostream &logWarn() { return std::cerr << "[Warning] "; }

inline std::ostream &logError() { return std::cerr << "[Error] "; }

// The following macros required 'verbosity' to be defined somewhere.
extern LogLevel verbosity;

inline bool checkVerbosity(LogLevel lvl) { return capi::verbosity >= lvl; }

#define LOG_CRITICAL(x) if (checkVerbosity(capi::LOG_CRITICAL)) { logInfo() << x;}
#define LOG_STATUS(x) if (checkVerbosity(capi::LOG_STATUS)) { logInfo() << x;}
#define LOG_EXTRA(x) if (checkVerbosity(capi::LOG_EXTRA)) { logInfo() << x;}

}

#define CAPI_DEFINE_VERBOSITY(lvl) namespace capi {LogLevel verbosity{lvl};}

#endif // CAPI_LOGGING_H
