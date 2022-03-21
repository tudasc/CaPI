//
// Created by sebastian on 21.03.22.
//

#include "XRayRuntime.h"
#include "../Utils.h"
#include "../selection/FunctionFilter.h"

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


// -- Symbol retrieving --

struct RemoveEnvInScope {

  explicit RemoveEnvInScope(const char* varName) : varName(varName) {
    oldVal = getenv(varName);
    if (oldVal)
      setenv(varName, "", true);
  }

  ~RemoveEnvInScope() {
    if (oldVal)
      setenv(varName, oldVal, true);
  }

private:
  const char* varName;
  const char* oldVal;
};


std::string getExecPath() {
  RemoveEnvInScope removePreload("LD_PRELOAD");
  char filename[128] = {0};
  auto n = readlink("/proc/self/exe", filename, sizeof(filename) - 1);
  if (n > 0) {
    return filename;
  }
  return "";
}

std::vector<MemMapEntry> readMemoryMap() {
  RemoveEnvInScope removePreload("LD_PRELOAD");

  std::vector<MemMapEntry> entries;

  char buffer[256];
  FILE *memory_map = fopen("/proc/self/maps", "r");
  if (!memory_map) {
    logInfo() << "Could not load memory map.\n";
  }

  std::string addrRange;
  std::string perms;
  uint64_t offset;
  std::string dev;
  uint64_t inode;
  std::string path;
  while (fgets(buffer, sizeof(buffer), memory_map)) {
    std::istringstream lineStream(buffer);
    lineStream >> addrRange >> perms >> std::hex >> offset >> std::dec >> dev >> inode >> path;
    //std::cout << "Memory Map: " << addrRange << ", " << perms << ", " << offset << ", " << path <<"\n";
    if (strcmp(perms.c_str(), "r-xp") != 0) {
      continue;
    }
    if (path.find(".so") == std::string::npos) {
      continue;
    }
    uintptr_t addrBegin = std::stoul(addrRange.substr(0, addrRange.find('-')), nullptr, 16);
    //std::cout << std::hex << addrBegin << '\n';
    entries.push_back({path, addrBegin, offset});
  }
  return entries;
}



SymbolTable loadSymbolTable(const std::string& object_file) {
  // Need to disable LD_PRELOAD, otherwise this library will be loaded in popen call.
  RemoveEnvInScope removePreload("LD_PRELOAD");

  std::string command = "nm --defined-only ";
  if (object_file.find(".so") != std::string::npos) {
    command += "-D ";
  }
  command += object_file;

  char buffer[256] = {0};
  FILE *output = popen(command.c_str(), "r");
  if (!output) {
    logError() << "Unable to execute nm to resolve symbol names.\n";
    return {};
  }

  SymbolTable table;

  uintptr_t addr;
  std::string symType;
  std::string symName;

  while (fgets(buffer, sizeof(buffer), output)) {
    std::istringstream line(buffer);
    if (buffer[0] != '0') {
      continue;
    }
    line >> std::hex >> addr;
    line >> symType;
    line >> symName;
    table[addr] = symName;
  }
  pclose(output);

  return table;

}

SymbolRetriever::SymbolRetriever(std::string execFile) : execFile(std::move(execFile)) {
}

bool SymbolRetriever::run() {
  loadSymTables();
  return true;
}

bool SymbolRetriever::loadSymTables() {

  addrToSymTable.clear();

  // Load symbols from main executable
  auto execSyms = loadSymbolTable(execFile);
  MappedSymTable execTable{std::move(execSyms), {execFile, 0, 0}};
  addrToSymTable[0] = execTable;

  // Load symbols from shared libs
  auto memMap = readMemoryMap();
  for (auto &entry: memMap) {
    auto &filename = entry.path;
    auto table = loadSymbolTable(filename);
    if (table.empty()) {
      logError() << "Could not load symbols from " << filename << "\n";
      continue;
    }

    //std::cout << "Loaded " << table.size() << " symbols from " << filename << "\n";
    //std::cout << "Starting address: " << entry.addrBegin << "\n";
    MappedSymTable mappedTable{std::move(table), entry};
    addrToSymTable[entry.addrBegin] = mappedTable;
    //        symTables.emplace_back(filename, std::move(mappedTable));
  }
  return true;
}

uint64_t mapAddrToProc(uint64_t addrInLib, const MappedSymTable& table) {
  return table.memMap.addrBegin + addrInLib - table.memMap.offset;
}

//
//


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
    }
  }

  logInfo() << "Functions found: " << numFound << "\n";
  logInfo() << "Functions registered: " << numInserted << "\n";
  logInfo() << "Functions patched: " << numPatched << "\n";

//  auto patchStatus = __xray_patch();
//  if (patchStatus != SUCCESS) {
//    logError() << "XRay patching failed\n";
//  }

}

}

namespace {
  struct InitXRay {
    InitXRay() { capi::initXRay(); }
  };

  InitXRay _;
}
