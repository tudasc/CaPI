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

#include "capi/support/Logging.h"
#include "capi/symbol_retriever/SymbolRetriever.h"

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

std::unique_ptr<capi::NeSmiKMode> mode{};
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


void ProfilingMode::handleRegionEnter(int id) XRAY_NEVER_INSTRUMENT {
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

void ProfilingMode::handleRegionExit(int id) XRAY_NEVER_INSTRUMENT {
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
    return;
  } else {
    // TODO: Find better solution, this is too expensive to call every time
    int mpiFinalized = 0;
    MPI_Finalized(&mpiFinalized);
    if (mpiFinalized) {
//      nesmik::finalize();
      finalized = true;
      return;
    }
  }

  if (!initialized) {
#ifdef WITH_MPI

    int mpiInited = 0;
    MPI_Initialized(&mpiInited);
    if (!mpiInited) {
      return;
    }
    logInfo() << "Initializing neSmiK\n";
//    nesmik::init();
    initialized = true;

#else
//    nesmik::init();
    initialized = true;
    // TODO: Should there be a non-MPI version?
#endif
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
    mode->handleRegionEnter(id);
    break;
  case XRayEntryType::TAIL:
  case XRayEntryType::EXIT:
    mode->handleRegionExit(id);
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

  bool shouldFilter = true;
  auto filterEnv = std::getenv("CAPI_DYNAMIC_FILTERING");
  if (filterEnv && (!strcmp(filterEnv, "0") || !strcmp(filterEnv, "OFF"))) {
    shouldFilter = false;
  }

  mode = std::make_unique<capi::ProfilingMode>(shouldFilter);

  logInfo() << "XRay initialization for neSmiK done.\n";
}

void preXRayFinalize() XRAY_NEVER_INSTRUMENT {
  logInfo() << "Finalizing XRay interface for neSmiK\n";
//  nesmik::finalize();
}

}
