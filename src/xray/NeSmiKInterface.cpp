//
// Created by sebastian on 30.08.22.
//
#include "NeSmiKInterface.h"
#include "XRayRuntime.h"
#include "CallLogger.h"

#include <cstring>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <fstream>

#include "capi/support/Logging.h"
#include "capi/symbol_retriever/SymbolRetriever.h"

#include "nlohmann/json.hpp"

#ifdef WITH_MPI
#include <mpi.h>
#endif

#include "nesmik.hpp"

namespace capi {
extern GlobalCaPIData *globalCaPIData;
}

namespace {

using RegionClock = std::chrono::high_resolution_clock;

constexpr long FILTERING_THRESHOLD_NANOS = 10000;
constexpr long FILTERING_MIN_INVOCATIONS = 100;

capi::Mode measurementMode{capi::Mode::PROFILE};
bool dynamicFiltering{false};
bool initialized{false};
bool finalized{false};
thread_local bool inXRayScope{false};

struct RegionMetrics {
  size_t numInvocations{0};
  long accumulatedTimeNanos{0};
  std::atomic_bool filtered{false};
  std::atomic_bool shouldMonitor{true};

  double meanDuration() const {
    return accumulatedTimeNanos / numInvocations;
  }
};

// FIXME: Is not thread safe!
std::unordered_map<int, RegionMetrics> regionMetricsMap;

thread_local std::unordered_map<int, std::vector<RegionClock::time_point>> timeStamps;

}

namespace capi {


static void handleRegionEnter(int id) XRAY_NEVER_INSTRUMENT {
  if (dynamicFiltering) {
    // FIXME: Thread-safety!
    auto& metrics = regionMetricsMap[id];
    if (metrics.filtered) {
      // TODO: Expected to be unpatched at this point
      return;
    }
    if (metrics.shouldMonitor) {
      timeStamps[id].push_back(RegionClock::now());
    }
  }
  auto& info = capi::globalCaPIData->xrayFuncMap[id];
  nesmik::region_start(info.name);
}

static void handleRegionExit(int id) XRAY_NEVER_INSTRUMENT {
  if (dynamicFiltering) {
    auto& metrics = regionMetricsMap[id];
    if (metrics.filtered) {
      // TODO: Expected to be unpatched at this point
      return;
    }
  }
  auto& info = capi::globalCaPIData->xrayFuncMap[id];
  nesmik::region_stop(info.name);

  if (dynamicFiltering) {
    auto& metrics = regionMetricsMap[id];
    if (metrics.shouldMonitor) {
      auto stopTime = RegionClock::now();
      auto& regionTimeStamps = timeStamps[id];
      if (regionTimeStamps.empty()) {
        logError() << "No time stamps for region " << info.name << "!\n";
        return;
      }
      auto startTime = regionTimeStamps.back();
      regionTimeStamps.pop_back();
      auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(stopTime - startTime);
      metrics.accumulatedTimeNanos += elapsed.count();
      metrics.numInvocations++;
      if (metrics.numInvocations >= FILTERING_MIN_INVOCATIONS) {
        metrics.shouldMonitor = false;
        if (metrics.meanDuration() < FILTERING_THRESHOLD_NANOS) {
          metrics.filtered = true;
          __xray_unpatch_function(id);
          logInfo() << "Region " << info.name << " filtered out! Mean time was " << metrics.meanDuration() << " ns over " << metrics.numInvocations << " invocations\n";
        }
      }
    }
  }
}

void handleXRayEvent(int32_t id, XRayEntryType type) XRAY_NEVER_INSTRUMENT {
  XRayRecursionGuard guard(inXRayScope);
  if (!guard) {
   logError() << "Recursive XRay event handling detected (id=" << id << ")!\n";
   return;
  }

  if (finalized) {
    auto& info = capi::globalCaPIData->xrayFuncMap[id];
    logError() << "Handling XRay event for function " << info.name << " (id=" << id << "): neSmiK interface has already been finalized.\n";
    return;
  }

  if (!initialized) {
    static bool failedBefore{false};
    if (!failedBefore) {
      auto& info = capi::globalCaPIData->xrayFuncMap[id];
      logError() << "Handling XRay event for function " << info.name << " (id=" << id << "): neSmiK interface has not been initialized.\n";
      failedBefore = true;
    }
    return;
  }

  switch (type) {
  case XRayEntryType::ENTRY:
    handleRegionEnter(id);
    break;
  case XRayEntryType::TAIL:
  case XRayEntryType::EXIT:
    handleRegionExit(id);
    break;
  default:
    logError() << "Unhandled XRay event type.\n";
    break;
  }
}

void postXRayInit(const XRayFunctionMap& xrayMap) XRAY_NEVER_INSTRUMENT {

  bool demangle = true;
  auto demangleEnv = std::getenv("CAPI_DEMANGLE");
  if (demangleEnv && (!strcmp(demangleEnv, "0") || !strcmp(demangleEnv, "OFF"))) {
    demangle = false;
  }

  measurementMode = Mode::PROFILE;
  auto modeEnv = std::getenv("CAPI_MODE");
  if (modeEnv && (!strcmp(modeEnv, "trace") || !strcmp(modeEnv, "TRACE"))) {
    measurementMode = Mode::TRACE;
  }

  // Dynamic filtering is only available in profiling mode.
  bool shouldFilter = measurementMode == Mode::PROFILE;
  if (shouldFilter) {
    // Can be turned off.
    auto filterEnv = std::getenv("CAPI_DYNAMIC_FILTERING");
    if (filterEnv && (!strcmp(filterEnv, "0") || !strcmp(filterEnv, "OFF"))) {
      shouldFilter = false;
    }
  }
  dynamicFiltering = shouldFilter;

  logInfo() << "XRay initialization for neSmiK done.\n";
  logInfo() << "Running in " << (measurementMode == Mode::PROFILE ? "profiling" : "tracing") << " mode.\n";
  logInfo() << "Dynamic filtering is " << (dynamicFiltering ? "enabled" : "disabled") << ".\n";
}

void preXRayFinalize() XRAY_NEVER_INSTRUMENT {
  logInfo() << "Finalizing XRay interface for neSmiK\n";
}

}

void dyncapi_nesmik_init() {
#ifdef WITH_MPI
  int mpiInited = 0;
  MPI_Initialized(&mpiInited);
  if (!mpiInited) {
    capi::logError() << "Called dyncapi_mpi_init before MPI_Init! This may lead to inconsistencies in the neSmiK profile.\n";
  }
#endif
  nesmik::init();
  initialized = true;
}

void dyncapi_nesmik_finalize() {
  __xray_unpatch();
  nesmik::finalize();
  finalized = true;
  if (dynamicFiltering) {

    std::vector<int> filtered;
    for (auto& [id, metrics] : regionMetricsMap) {
      if (metrics.filtered) {
        filtered.push_back(id);
      }
    }

  #ifdef WITH_MPI
    int mpiFinalized = 0;
    MPI_Finalized(&mpiFinalized);
    if (mpiFinalized) {
      capi::logError() << "dyncapi_mpi_finalize was called after MPI was finalized. Unable to gather filtered regions.\n";
      return;
    }

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) capi::logInfo() << "Gathering filtered regions in MPI rank 0...\n";

    // Gathering filtered IDs in rank 0
    int localSize = filtered.size();

    // Gather sizes first
    std::vector<int> recvCounts;
    if (rank == 0) recvCounts.resize(size);
    MPI_Gather(&localSize, 1, MPI_INT,
               recvCounts.data(), 1, MPI_INT,
               0, MPI_COMM_WORLD);

    // Calculate displacements for the receive buffer
    std::vector<int> displs;
    int totalCount = 0;
    if (rank == 0) {
      displs.resize(size);
      for (int i = 0; i < size; ++i) {
        displs[i] = totalCount;
        totalCount += recvCounts[i];
      }
    }

    // Gather all data
    std::vector<int> gathered;
    if (rank == 0) gathered.resize(totalCount);

    MPI_Gatherv(filtered.data(), localSize, MPI_INT,
                gathered.data(), recvCounts.data(), displs.data(), MPI_INT,
                0, MPI_COMM_WORLD);

    std::unordered_set<int> gatheredSet;
    gatheredSet.insert(gathered.begin(), gathered.end());

    filtered.clear();
    filtered.reserve(gatheredSet.size());
    std::copy(gatheredSet.begin(), gatheredSet.end(), std::back_inserter(filtered));
  #endif

    // Convert to names
    std::vector<std::string> filteredNames;
    for (auto id : filtered) {
      auto& info = capi::globalCaPIData->xrayFuncMap[id];
      filteredNames.push_back(info.name);
    }

    // Write to file
#ifdef WITH_MPI
    if (rank == 0) {
#endif
      // Convert to JSON
      nlohmann::json j = filteredNames;

      auto execPath = getExecPath();
      auto execFilename = execPath.substr(execPath.find_last_of('/') + 1);

      auto outFile = execFilename + ".filtered.json";

      // Write to file
      std::ofstream out(outFile);
      out << j.dump(2) << std::endl;

      capi::logInfo() << "A list of all dynamically filtered regions has been written to " << outFile << "\n";

#ifdef WITH_MPI
    }
#endif

  }

}
