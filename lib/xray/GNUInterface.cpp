//
// Created by sebastian on 30.08.22.
//

#include "../Utils.h"
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

namespace {
  // It's faster to cache here than to go over the XRay runtime every time.
  std::unordered_map<int, uintptr_t> id2Addr;
}

namespace capi {

void handleXRayEvent(int32_t id, XRayEntryType type) XRAY_NEVER_INSTRUMENT {

  auto& addrEntry = id2Addr[id];
  if (!addrEntry) {
    uintptr_t addr = __xray_function_address(id);
    if (!addr) {
      logError() << "Event from unknown address\n";
      return;
    }
    addrEntry = addr;
  }

  switch(type) {
  case XRayEntryType::ENTRY:
    __cyg_profile_func_enter(reinterpret_cast<void*>(addrEntry), nullptr);
    break;
  case XRayEntryType::TAIL:
  case XRayEntryType::EXIT:
    __cyg_profile_func_exit(reinterpret_cast<void*>(addrEntry), nullptr);
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