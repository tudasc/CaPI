//
// Created by sebastian on 21.03.22.
//

#include "XRayRuntime.h"
#include "../Utils.h"
#include "../selection/FunctionFilter.h"
#include "SymbolRetriever.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <unistd.h>

#include "xray/xray_interface.h"


namespace capi {

extern void handleXRayEvent(int32_t id, XRayEntryType type);

extern void postXRayInit(const SymbolTable&);
bool isDone(int i);
void initXRay() XRAY_NEVER_INSTRUMENT {

  Timer timer("[Info] Initialization took ", std::cout);

  bool shouldInit{false};

  auto enableEnv = std::getenv("CAPI_ENABLE");
  if (enableEnv) {
    shouldInit = true;
  }

  bool noFilter{true};
  FunctionFilter filter;
  auto filterEnv = std::getenv("CAPI_FILTERING_FILE");
  if (filterEnv) {
    if (readScorePFilterFile(filter, filterEnv)) {
      logInfo() << "Loaded filter file with " << filter.size() << " entries.\n";
      noFilter = false;
      shouldInit = true;
    } else {
      logError() << "Failed to read filter file from " << filterEnv << "\n";
      return;
    }
  } else {
    logInfo() << "No CaPI filtering file specified.\n";
  }

  if (!shouldInit) {
    logInfo() << "CaPI is inactive. Pass CAPI_FILTERING_FILE or set CAPI_ENABLE=1 if you want to active instrumentation.\n";
  }

  auto execPath = getExecPath();
  auto execFilename = execPath.substr(execPath.find_last_of('/') + 1);

  auto symTables = loadMappedSymTables(execPath);

  size_t numFound = 0;
  size_t numInserted = 0;
  size_t numPatched = 0;
  size_t numFailed = 0;

  SymbolTable globalTable;
  std::unordered_set<uintptr_t> filteredOut;

  for (auto&& [startAddr, table] : symTables) {
    for (auto&& [addr, symName] : table.table) {
      auto addrInProc = mapAddrToProc(addr, table);
      if (noFilter || filter.accepts(symName)) {
        globalTable[addrInProc] = symName;
        ++numInserted;
      } else {
        filteredOut.insert(addrInProc);
      }
      ++numFound;
    }
  }

  __xray_init();
  __xray_set_handler(handleXRayEvent);

  for (int objId = 0; objId < __xray_num_objects(); ++objId) {
    size_t maxFID = __xray_max_function_id_in_object(objId);
    if (maxFID == 0) {
      logError() << "No functions instrumented.\n";
      return;
    }

    logInfo() << "Detected " << maxFID << " patchable functions in object " << objId << "\n";

    for (int fid = 1; fid <= maxFID; ++fid) {
      uintptr_t addr = __xray_function_address_in_object(fid, objId);
      if (!addr) {
        logError() << "Unable to determine address for function " << fid << "\n";
        continue;
      }
      if (filteredOut.find(addr) != filteredOut.end()) {
        continue;
      }
      if (auto it = globalTable.find(addr); it != globalTable.end()) {
        auto patchStatus = __xray_patch_function_in_object(fid, objId);
        if (patchStatus == SUCCESS) {
          //logInfo() << "Patched function " << std::hex << addr << std::dec << ": id=" << fid << ", name=" << it->second << "\n";
          numPatched++;
        } else {
          logError() << "XRay patching at " << std::hex << addr << std::dec << " failed: id=" << fid << ", name=" << it->second << "\n";
          numFailed++;
        }
      } else {
        logError() << "Unable to find symbol for patchable function: id=" << fid << ", addr=" << std::hex << addr << std::dec << "\n";
        numFailed++;
      }
    }
  }

  logInfo() << "Functions found: " << numFound << "\n";
  logInfo() << "Functions registered: " << numInserted << "\n";
  logInfo() << "Functions patched: " << numPatched << " (" << numFailed << " failed)\n";

  postXRayInit(globalTable);

}

}

namespace {
  struct InitXRay {
    InitXRay() { capi::initXRay(); }
  };

  InitXRay _;
}
