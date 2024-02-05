//
// Created by sebastian on 30.08.22.
//

#include "XRayRuntime.h"
#include "CallLogger.h"

#include <cstring>
#include <unordered_map>

#include "support/Logging.h"
#include "SymbolRetriever.h"

#ifdef WITH_MPI
#include <mpi.h>
#endif


extern "C" {
void Extrae_init() __attribute__((weak));
int Extrae_is_initialized() __attribute__((weak));
void Extrae_event(unsigned type, long long value) __attribute__((weak));
void Extrae_eventandcounters(unsigned type, long long value) __attribute__((weak));
void Extrae_define_event_type(unsigned *type, char *type_description, int *nvalues,
                              long long *values, char **values_description) __attribute__((weak));
}

namespace capi {
extern GlobalCaPIData *globalCaPIData;
}

namespace {

unsigned int ExtraeXRayEvt = 31169;

bool recordOutsideMPI = true;

bool initialized{false};

thread_local std::vector<int> callStack{};
thread_local bool globalActive{false};
thread_local bool threadInitialized{false};
thread_local bool scopeActive{false};

inline void handle_extrae_region_enter(int id) XRAY_NEVER_INSTRUMENT {

  if (capi::globalCaPIData->useScopeTriggers && !scopeActive) {
    if (capi::globalCaPIData->scopeTriggerSet.find(id) != capi::globalCaPIData->scopeTriggerSet.end()) {
      scopeActive = true;
    } else {
      return;
    }
  }
  Extrae_eventandcounters(ExtraeXRayEvt, id);
  if (capi::globalCaPIData->logCalls) {
    auto& info = capi::globalCaPIData->xrayFuncMap[id];
    capi::globalCaPIData->logger->logEnter(callStack.size(), info);
  }
  callStack.push_back(id);
}

inline void handle_extrae_region_exit(int id) XRAY_NEVER_INSTRUMENT {
  if (capi::globalCaPIData->useScopeTriggers && !scopeActive) {
    return;
  }
  int nextEvt = 0;
  callStack.pop_back();
  if (!callStack.empty()) {
    nextEvt = callStack.back();
  } else if (capi::globalCaPIData->useScopeTriggers) {
    // We don't record the call stack before the triggering function.
    // Therefore, it is always at the top of the call chain.
    scopeActive = false;
  }
  Extrae_eventandcounters(ExtraeXRayEvt, nextEvt);
  if (capi::globalCaPIData->logCalls) {
    auto& info = capi::globalCaPIData->xrayFuncMap[id];
    capi::globalCaPIData->logger->logExit(callStack.size(), info);
  }
}

}

namespace capi {

void handleXRayEvent(int32_t id, XRayEntryType type) XRAY_NEVER_INSTRUMENT {

  if (!initialized) {
    static bool failedBefore{false};
    if (!failedBefore) {
      logError() << "Extrae interface has not been initialized.\n";
      failedBefore = true;
    }
    return;
  }
  if (!threadInitialized) {
    globalActive = globalCaPIData->beginActive;
    threadInitialized = true;
  }

  // TODO: Long term, use different event handler function when patching to reduce lookup overhead
  if (globalActive) {
    if (type == XRayEntryType::EXIT && globalCaPIData->endTriggerSet.find(id) == globalCaPIData->endTriggerSet.end()) {
      globalActive = false;
      // no immediate return here, since we first want to record the exit event
    }
  } else {
    if (type == XRayEntryType::ENTRY && globalCaPIData->beginTriggerSet.find(id) == globalCaPIData->beginTriggerSet.end()) {
      globalActive = true;
    } else {
      return;
    }   
  }  

  #ifdef WITH_MPI
  if (!recordOutsideMPI) {
    int mpiInited = 0, mpiFinalized = 0;
    MPI_Initialized(&mpiInited);
    MPI_Finalized(&mpiFinalized);
    if (!mpiInited || mpiFinalized) {
      return;
    }
  }
  #endif

  switch (type) {
  case XRayEntryType::ENTRY:
    handle_extrae_region_enter(id);
    break;
  case XRayEntryType::TAIL:
  case XRayEntryType::EXIT:
    handle_extrae_region_exit(id);
    break;
  default:
    logError() << "Unhandled XRay event type.\n";
    break;
  }
}

void registerExtraeEvents(const XRayFunctionMap& xrayMap, bool demangle) XRAY_NEVER_INSTRUMENT {
  int nvalues = xrayMap.size();
  std::vector<long long> idList;
  idList.reserve(nvalues);
  std::vector<char*> descriptions;
  descriptions.reserve(nvalues);
  for (auto&& [id, info] : xrayMap) {
    idList.push_back(id);
    // Const-casting is ugly but necessary, if we want to avoid copying all function names
    descriptions.push_back(const_cast<char*>(demangle ? info.demangled.c_str() : info.name.c_str()));
  }

  Extrae_define_event_type (&ExtraeXRayEvt, const_cast<char*>("XRay Event"), &nvalues, &idList[0], &descriptions[0]);
}

void postXRayInit(const XRayFunctionMap& xrayMap) XRAY_NEVER_INSTRUMENT {
  if (!Extrae_init) {
    logError() << "Extrae API not available. XRay events will not be traced.\n";
    return;
  }
  if (!Extrae_is_initialized()) {
    Extrae_init();
    if (!Extrae_is_initialized()) {
      logError()
          << "Failed to initialize Extrae. XRay events will not be traced.\n";
      return;
    }
  }
  bool demangle = true;
  auto demangleEnv = std::getenv("CAPI_DEMANGLE");
  if (demangleEnv && (!strcmp(demangleEnv, "0") || !strcmp(demangleEnv, "OFF"))) {
    demangle = false;
  }

  registerExtraeEvents(xrayMap, demangle);

  initialized = true;
  logInfo() << "XRay initialization and Extrae event registration done.\n";
}

void preXRayFinalize() XRAY_NEVER_INSTRUMENT {
  logInfo() << "Finalizing XRay interface for Extrae" << std::endl;
}

}
