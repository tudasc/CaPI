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
#include <thread>

#include "capi/nesmik_merger/NesmikMerger.h"
#include "capi/support/Logging.h"
#include "capi/symbol_retriever/SymbolRetriever.h"

#include "io/VersionFourMCGWriter.h"

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

    constexpr long FILTERING_THRESHOLD_MICROS_DFLT = 10;
    constexpr long FILTERING_MIN_INVOCATIONS_DFLT = 100;

    long filteringThresholdMicros = FILTERING_THRESHOLD_MICROS_DFLT;
    long filteringMinInvocs = FILTERING_MIN_INVOCATIONS_DFLT;

    capi::Mode measurementMode{capi::Mode::PROFILE};
    bool dynamicFiltering{false};
    bool initialized{false};
    bool finalized{false};
    bool recordInParallelRegions{false};
    bool recordOnlyMainThread{true};
    bool isRank0{true};
    thread_local bool inXRayScope{false};
    thread_local bool inParallelRegion{false};
    thread_local int functionLastEnteredBeforeInit{-1};

    struct RegionMetrics {
        size_t numInvocations{0};
        long accumulatedTimeNanos{0};
        std::atomic_bool filtered{false};
        std::atomic_bool shouldMonitor{true};

        double meanDuration() const {
            return accumulatedTimeNanos / numInvocations;
        }

        double meanDurationMicros() const {
            return meanDuration() / 1000;
        }
    };

// FIXME: Is not thread safe!
    std::unordered_map<int, RegionMetrics> regionMetricsMap;

    thread_local std::unordered_map<int, std::vector<RegionClock::time_point>> timeStamps;

    pid_t mainThreadId;

    struct ThreadGuard {

        explicit ThreadGuard(pid_t mainThread) {
            auto thisThread = gettid();
            this->isMainThread = thisThread == mainThread;
        }

        operator bool() const {
            return check();
        }

        bool check() const {
            return isMainThread;
        }

    private:
        bool isMainThread;
    };

    std::string parseTalpOutputFile(const std::string &dlbArgs) {
        const std::string key = "--talp-output-file=";
        size_t pos = dlbArgs.find(key);
        if (pos == std::string::npos) {
            return "";
        }

        pos += key.length();
        size_t endPos = dlbArgs.find_first_of(" \t", pos);
        if (endPos == std::string::npos) {
            endPos = dlbArgs.length();
        }

        return dlbArgs.substr(pos, endPos - pos);
    }

    std::string getTalpOutputFile() {
        const char *defaultFile = "talp.json";

        const char *envValue = std::getenv("DLB_ARGS");
        if (!envValue) {
            return defaultFile;
        }

        std::string dlbArgs(envValue);
        std::string filename = parseTalpOutputFile(dlbArgs);

        if (filename.empty()) {
            return defaultFile;
        }
        return filename;
    }
}

namespace capi {


void registerExtraOptions(cxxopts::Options& options) {
  options.add_options()
      ("mode", "Runtime mode: profile|trace",
       cxxopts::value<std::string>()->default_value("profile"))
      ("dynamic-filtering", "Enable dynamic filtering",
       cxxopts::value<bool>()->default_value("true"))
      ("filter-limit-micros", "Filter threshold in microseconds",
       cxxopts::value<int>()->default_value(std::to_string(FILTERING_THRESHOLD_MICROS_DFLT)))
      ("filter-min-calls", "Minimum calls before filtering",
       cxxopts::value<int>()->default_value(std::to_string(FILTERING_MIN_INVOCATIONS_DFLT)))
      ("profile-name", "Filename for the generated TALP profile",
       cxxopts::value<std::string>()->default_value("talp_metrics.mcg"))
      ("validate", "Run static call graph validation after termination",
       cxxopts::value<bool>()->default_value("false"));
}



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
      if (metrics.numInvocations >= filteringMinInvocs) {
        metrics.shouldMonitor = false;
        if (metrics.meanDurationMicros() < filteringThresholdMicros) {
          metrics.filtered = true;
          __xray_unpatch_function(id);
//          logInfo() << "Region " << info.name << " filtered out! Mean time was " << metrics.meanDuration() << " ns over " << metrics.numInvocations << " invocations\n";
        }
      }
    }
  }
}

void handleCustomXRayEvent(void* data, size_t len) {
  const char* eventName = static_cast<const char*>(data);
  if (eventName[len-1] != '\0') {
    logError() << "Custom XRay event is not a string!\n";
    return;
  }
  if (!strcmp(eventName, "dyncapi_init")) {
    dyncapi_nesmik_init();
  } else if (!strcmp(eventName, "dyncapi_finalize")) {
    dyncapi_nesmik_finalize();
  } else if (!strcmp(eventName, "dyncapi_par_region_enter")) {
    dyncapi_par_region_enter();
  } else if (!strcmp(eventName, "dyncapi_par_region_exit")) {
    dyncapi_par_region_exit();
  } else {
    logError() << "Received unknown custom XRay event: " << eventName << "\n";
  }
}


void handleXRayEvent(int32_t id, XRayEntryType type) XRAY_NEVER_INSTRUMENT {
  XRayRecursionGuard guard(inXRayScope);
  if (!guard) {
   logError() << "Recursive XRay event handling detected (id=" << id << ") - unpatching...\n";
   __xray_unpatch_function(id);
   // Because we're unpatched, it should be safe to access the function info now.
   auto& info = capi::globalCaPIData->xrayFuncMap[id];
   logError() << "Offending function was '" << info.name << "'. Consider filtering this function statically.\n";
   return;
  }

  if (recordOnlyMainThread) {
      thread_local ThreadGuard threadGuard(mainThreadId);
      if (!threadGuard) {
          return;
      }
  }

    if (!recordInParallelRegions && inParallelRegion) {
    return;
  }

  if (finalized) {
    auto& info = capi::globalCaPIData->xrayFuncMap[id];
    logError() << "Handling XRay event for function " << info.name << " (id=" << id << "): neSmiK interface has already been finalized.\n";
    return;
  }

  if (!initialized) {
    functionLastEnteredBeforeInit = id;
    static bool failedBefore{false};
    if (!failedBefore) {
      auto& info = capi::globalCaPIData->xrayFuncMap[id];
      logWarn() << "Handling XRay event for function " << info.name << " (id=" << id << "): neSmiK interface has not been initialized.\n";
      failedBefore = true;
    }
    return;
  }

  // To avoid inconsistencies, we ignore all invocations of the function from which neSmiK was initialized.
  // Otherwise, if this function is instrumented, the entry event will not be recorded but the exit will be.
  if (functionLastEnteredBeforeInit == id) {
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

void postXRayInit() XRAY_NEVER_INSTRUMENT {
  mainThreadId = gettid();

  auto opts = globalCaPIData->options;

  filteringThresholdMicros = opts["filter-limit-micros"].as<int>();
  filteringMinInvocs = opts["filter-min-calls"].as<int>();

  const char* nesmikBackend;

  const auto& modeStr = opts["mode"].as<std::string>();
  if (modeStr == "profile") {
    measurementMode = Mode::PROFILE;
    nesmikBackend = "CaPI";
  } else if (modeStr == "trace") {
    measurementMode = Mode::TRACE;
    nesmikBackend = "Extrae::TypeStack";
  } else {
    logError() << "Invalid mode selected. Defaulting to PROFILE.\n";
    measurementMode = Mode::PROFILE;
    nesmikBackend = "CaPI";
  }

  // Dynamic filtering is only available in profiling mode.
  dynamicFiltering = (measurementMode == Mode::PROFILE) && opts["dynamic-filtering"].as<bool>();

  // TODO: Make this configurable?
  recordInParallelRegions = false;

  // Set NeSmiK backend (if not already set explicitly)
  setenv("NESMIK_BACKEND", nesmikBackend, 1);

  logInfo() << "XRay initialization for neSmiK done.\n";
  logInfo() << "Running in " << (measurementMode == Mode::PROFILE ? "profiling" : "tracing") << " mode with neSmiK backend '" << nesmikBackend << "'.\n";
  logInfo() << "Dynamic filtering is " << (dynamicFiltering ? "enabled" : "disabled") << ".\n";
}

void preXRayFinalize() XRAY_NEVER_INSTRUMENT {
  logInfo() << "Finalizing XRay interface for neSmiK\n";
    if (initialized && measurementMode == Mode::PROFILE && isRank0) {
        capi::logInfo() << "Merging TALP metrics with static graph...\n";
        // Generate metric profile
        auto staticCg = capi::globalCaPIData->runtimeGraph->getStaticGraph();

        std::ifstream talp_json_file(getTalpOutputFile());
        nlohmann::json talp_json;
        talp_json_file >> talp_json;

        std::ifstream nesmik_json_file("capinesmik.json");
        nlohmann::json nesmik_json;
        nesmik_json_file >> nesmik_json;

        auto metricCg = capi::buildDynamicGraphAndAttachMetrics(staticCg, talp_json, nesmik_json, capi::globalCaPIData->finalDynamicFilterSet);

        if (metricCg) {
            const auto& outFile = capi::globalCaPIData->options["profile-name"].as<std::string>();
            auto mcgWriter = io::createWriter(4);
            if (mcgWriter) {
                io::JsonSink jsonSink;
                mcgWriter->write(metricCg.get(), jsonSink);
                std::ofstream os(outFile);
                os << jsonSink.getJson().dump(4) << std::endl;
                capi::logInfo() << "Profile successfully written to " << outFile << "!\n";
            } else {
                capi::logError() << "Unable to create a writer for MetaCG format version 4\n";
            }

        } else {
            capi::logError() << "Failed to merge TALP profile!\n";
        }
    }

}

}



void  __attribute__((visibility("default")))  dyncapi_nesmik_init() XRAY_NEVER_INSTRUMENT {
#ifdef WITH_MPI
  int mpiInited = 0;
  MPI_Initialized(&mpiInited);
  if (!mpiInited) {
    capi::logError() << "Called dyncapi_mpi_init before MPI_Init! This may lead to inconsistencies in the neSmiK profile.\n";
  } else {
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank != 0) {
      isRank0 = false;
    }
  }
#endif
  nesmik::init();
  initialized = true;
}

void  __attribute__((visibility("default")))  dyncapi_nesmik_finalize() XRAY_NEVER_INSTRUMENT {
  __xray_unpatch();
  nesmik::finalize();
  finalized = true;

  if (!initialized) {
      capi::logError() << "NeSmiK was not initialized, but finalize was called!\n";
      return;
  }

  std::unordered_set<int> filteredSet;
  std::vector<int> filtered;

  std::unordered_set<std::string> filteredNamesSet;

  if (dynamicFiltering) {


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

    if (rank == 0) {
        capi::logInfo() << "Gathering filtered regions in MPI rank 0...\n";
    }

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

    filteredSet.insert(gathered.begin(), gathered.end());

  #else
      filteredSet.insert(filtered.begin(), filtered.end());
  #endif

    // Convert to names
    for (auto id : filteredSet) {
      auto& info = capi::globalCaPIData->xrayFuncMap[id];
      filteredNamesSet.insert(info.name);
    }

    // Write to file
      if(isRank0) {
          // Convert to JSON
          nlohmann::json j = filteredNamesSet;

          auto execPath = getExecPath();
          auto execFilename = execPath.substr(execPath.find_last_of('/') + 1);

          auto outFile = execFilename + ".filtered.json";

          // Write to file
          std::ofstream out(outFile);
          out << j.dump(2) << std::endl;

          capi::logInfo() << "Dynamic filtering disabled " << filteredSet.size() << " regions.\n";
          capi::logInfo() << "A list of all dynamically filtered regions has been written to " << outFile << "\n";

          capi::globalCaPIData->finalDynamicFilterSet = std::move(filteredNamesSet);

      }

  }

    if(isRank0) {
        capi::globalCaPIData->runtimeGraph->printStats();
        bool shouldValidate = capi::globalCaPIData->options["validate"].as<bool>();

        if (shouldValidate && capi::globalCaPIData->measurementConfig) {
            capi::globalCaPIData->runtimeGraph->validateQuery(*capi::globalCaPIData->measurementConfig);
        }
    }

}

void __attribute__((visibility("default")))  dyncapi_par_region_enter() XRAY_NEVER_INSTRUMENT {
  inParallelRegion = true;
}

void  __attribute__((visibility("default"))) dyncapi_par_region_exit() XRAY_NEVER_INSTRUMENT{
  inParallelRegion = false;
}
