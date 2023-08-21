//
// Created by sebastian on 14.07.23.
//

#include "CallLogger.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "support/Logging.h"

#ifdef WITH_MPI
#include <mpi.h>
#endif

namespace capi {

void CallLogger::makeLogEntry(int depth, std::string_view type, const capi::XRayFunctionInfo& info) {
  if (!fileIsSet) {
    determineLogfile();
  }
  std::stringstream ss;
  while (depth-- > 0) {
    ss << "  ";
  }
  ss << type << " " << info.name;
  buffer.push_back(ss.str());
  if (buffer.size() >= bufferCapacity) {
    flush();
  }
}

void CallLogger::logEnter(int depth, const capi::XRayFunctionInfo &info) {
  makeLogEntry(depth, "ENTER", info);
}

void CallLogger::logExit(int depth, const capi::XRayFunctionInfo &info) {
  makeLogEntry(depth, "EXIT", info);
}

void CallLogger::determineLogfile() {
  if (!fileIsSet) {
    int tries = 0;
    std::string logFile;
    std::string prefix = basename;
#ifdef WITH_MPI
    int mpiFinalized = 0;
    MPI_Finalized(&mpiFinalized);
    if (mpiFinalized) {
      return;
    }
    int mpiInited = 0;
    MPI_Initialized(&mpiInited);
    if (!mpiInited) {
      return;
    }
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    prefix += ".proc" + std::to_string(rank);
#endif
    do {
      logFile = prefix + ".log" + (tries++ == 0 ? "" : "." + std::to_string(tries));
    } while (std::filesystem::exists(logFile));
    this->logfile = logFile;
    logInfo() << "Writing call log to " << logFile << "\n";
    fileIsSet = true;
  }
}

void CallLogger::flush() {
  determineLogfile();
  if (!fileIsSet) {
    logInfo() << "Could not determine a suitable log file - maybe there was no log entry in MPI scope?.\n";
    return;
  }
  std::ofstream ofs(logfile, std::ios_base::out | std::ios_base::app);
  if (!ofs.is_open()) {
    logError() << "Unable to flush call log to file " << logfile << std::endl;
    return;
  }
  for (auto& logEntry : buffer) {
    ofs << logEntry << "\n";
  }
  ofs.flush();
  buffer.clear();
}

}