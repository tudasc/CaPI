//
// Created by sebastian on 21.08.23.
//

#ifndef CAPI_LOGGING_H
#define CAPI_LOGGING_H

#include <iostream>
#include <unistd.h>

#ifdef WITH_MPI
#include <mpi.h>
#endif

namespace capi {

enum LogLevel { LOG_NONE, LOG_CRITICAL, LOG_STATUS, LOG_EXTRA };

#ifdef WITH_MPI
inline int getMPIRank() {
  static int rank = -1;
  static bool cached = false;

  if (!cached) {
    int initialized = 0;
    MPI_Initialized(&initialized);
    if (initialized) {
      int finalized = 1;
      MPI_Finalized(&finalized);
      if (!finalized) {
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        cached = true;
      }
    }
  }
  return rank; // -1 if MPI not initialized yet
}
#endif

inline std::ostream &logPrefix(std::ostream &os, const char *level) {
#ifdef WITH_MPI
  int rank = getMPIRank();
  if (rank >= 0) {
    os << "[Rank " << rank << "] ";
  } else {
    os << "[PID " << getpid() << "] ";
  }
#endif
  os << level;
  return os;
}

inline std::ostream &logInfo() {
  return logPrefix(std::cout, "[Info] ");
}

inline std::ostream &logWarn() {
  return logPrefix(std::cerr, "[Warning] ");
}

inline std::ostream &logError() {
  return logPrefix(std::cerr, "[Error] ");
}

// The following macros required 'verbosity' to be defined somewhere.
extern LogLevel verbosity;

inline bool checkVerbosity(LogLevel lvl) { return capi::verbosity >= lvl; }

#define LOG_CRITICAL(x) if (checkVerbosity(capi::LOG_CRITICAL)) { logInfo() << x;}
#define LOG_STATUS(x) if (checkVerbosity(capi::LOG_STATUS)) { logInfo() << x;}
#define LOG_EXTRA(x) if (checkVerbosity(capi::LOG_EXTRA)) { logInfo() << x;}

}

#define CAPI_DEFINE_VERBOSITY(lvl) namespace capi {LogLevel verbosity{lvl};}

#endif // CAPI_LOGGING_H
