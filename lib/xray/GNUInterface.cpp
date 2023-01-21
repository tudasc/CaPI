//
// Created by sebastian on 30.08.22.
//

#include "../Utils.h"
#include "SymbolRetriever.h"
#include "XRayRuntime.h"

#ifdef CAPI_SCOREP_INTERFACE
#include "ScorePInit.h"
#endif

extern "C" {
void __cyg_profile_func_enter(void* fn, void* callsite) XRAY_NEVER_INSTRUMENT;
void __cyg_profile_func_exit(void* fn, void* callsite) XRAY_NEVER_INSTRUMENT;

};

namespace capi {

void handleXRayEvent(int32_t id, XRayEntryType type) XRAY_NEVER_INSTRUMENT {
  uintptr_t addr = __xray_function_address(id);
  if (!addr) {
    return;
  }

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

void postXRayInit(const SymbolTable &symTable) {
#ifdef CAPI_SCOREP_INTERFACE
      initScoreP(symTable);
#endif
}

}