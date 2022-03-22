//
// Created by sebastian on 21.03.22.
//

#include "XRayRuntime.h"
#include "SymbolRetriever.h"
#include "../Utils.h"
#include "../selection/FunctionFilter.h"

#ifdef CAPI_SCOREP_SUPPORT
#include "ScorePInit.h"
#endif

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unistd.h>

#include "xray/xray_interface.h"

#define XRAY_NEVER_INSTRUMENT __attribute__((xray_never_instrument))


extern "C" {
void __cyg_profile_func_enter(void* fn, void* callsite) XRAY_NEVER_INSTRUMENT;
void __cyg_profile_func_exit(void* fn, void* callsite) XRAY_NEVER_INSTRUMENT;

};


namespace capi {


using XRayHandlerFn = void (*)(int32_t, XRayEntryType);

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


void initXRay() XRAY_NEVER_INSTRUMENT {

  Timer timer("[Info] Initialization took ", std::cout);

  bool noFilter{true};
  FunctionFilter filter;
  auto filterEnv = std::getenv("CAPI_FILTERING_FILE");
  if (filterEnv) {
    if (readScorePFilterFile(filter, filterEnv)) {
      logInfo() << "Loaded filter file with " << filter.size() << " entries.\n";
      noFilter = false;
    } else {
      logError() << "Failed to read filter file from " << filterEnv << "\n";
      return;
    }
  }

  auto execPath = getExecPath();
  auto execFilename = execPath.substr(execPath.find_last_of('/') + 1);

  SymbolRetriever symRetriever(execFilename);
  symRetriever.run();

  auto& symTables = symRetriever.getMappedSymTables();

  size_t numFound = 0;
  size_t numInserted = 0;
  size_t numPatched = 0;

  SymbolTable globalTable;

  for (auto&& [startAddr, table] : symTables) {
    for (auto&& [addr, symName] : table.table) {
      auto addrInProc = mapAddrToProc(addr, table);
      if (noFilter || filter.accepts(symName)) {
        globalTable[addrInProc] = symName;
        ++numInserted;
      }
      ++numFound;
    }
  }

  __xray_init();
  __xray_set_handler(handleXRayEvent);

  size_t maxFID = __xray_max_function_id();
  if (maxFID == 0) {
    logError() << "No functions instrumented.\n";
    return;
  }

  logInfo() << "Patching " << maxFID << " functions\n";

  for (int fid = 1; fid <= maxFID; ++fid) {
    uintptr_t addr = __xray_function_address(fid);
    if (!addr) {
      logError() << "Unable to determine address for function " << fid << "\n";
      continue;
    }
    if (auto it = globalTable.find(addr); it != globalTable.end()) {
      auto patchStatus = __xray_patch_function(fid);
      if (patchStatus == SUCCESS) {
        logInfo() << "Patched function: id=" << fid << ", name=" << it->second << "\n";
        numPatched++;
      } else {
        logError() << "XRay patching failed: id=" << fid << ", name=" << it->second << "\n";
      }
    } else {
      logError() << "Unable find symbol for function: id=" << fid << ", addr=" << std::hex << addr << std::dec << "\n";
    }
  }

  logInfo() << "Functions found: " << numFound << "\n";
  logInfo() << "Functions registered: " << numInserted << "\n";
  logInfo() << "Functions patched: " << numPatched << "\n";

#ifdef CAPI_SCOREP_SUPPORT
  initScoreP(globalTable);
#endif

}

}

namespace {
  struct InitXRay {
    InitXRay() { capi::initXRay(); }
  };

  InitXRay _;
}
