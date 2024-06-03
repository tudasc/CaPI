//
// Created by sebastian on 30.08.22.
//

#include "support/Logging.h"
#include "SymbolRetriever.h"
#include "XRayRuntime.h"

#ifdef CAPI_SCOREP_INTERFACE
#include "ScorePInit.h"
#endif

#include <unordered_map>

extern "C" {
void __cyg_profile_func_enter(void* fn, void* callsite) XRAY_NEVER_INSTRUMENT;
void __cyg_profile_func_exit(void* fn, void* callsite) XRAY_NEVER_INSTRUMENT;

};


namespace capi {

extern capi::GlobalCaPIData* globalCaPIData;

void handleXRayEvent(int32_t id, XRayEntryType type) XRAY_NEVER_INSTRUMENT {
  // We assume here that XRay is successfully initialized, otherwise this function should not be called
  auto& xrayMap = globalCaPIData->xrayFuncMap;
  auto it = xrayMap.find(id);
  if (it == xrayMap.end()) {
    logError() << "Event from unknown address\n";
    return;
  }
  uintptr_t addr = it->second.addr;

  switch(type) {
  case XRayEntryType::ENTRY:
    __cyg_profile_func_enter(reinterpret_cast<void*>(addr), nullptr);
    break;
  case XRayEntryType::TAIL:
  case XRayEntryType::EXIT:
    __cyg_profile_func_exit(reinterpret_cast<void*>(addr), nullptr);
    break;
  default:
    logError() << "Unhandled XRay event type.\n";
    break;
  }

}

void postXRayInit(const XRayFunctionMap &xrayFuncMap) XRAY_NEVER_INSTRUMENT {
#ifdef CAPI_SCOREP_INTERFACE
      initScoreP(xrayFuncMap);
#endif
}

void preXRayFinalize() XRAY_NEVER_INSTRUMENT {
#ifdef CAPI_SCOREP_INTERFACE
      finalizeScoreP();
#endif
}

}