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
#include <unistd.h>
#include <unordered_set>

#include "xray/xray_interface.h"

#include "llvm/XRay/InstrumentationMap.h"
#include "llvm/DebugInfo/Symbolize/Symbolize.h"
#include "llvm/ADT/DenseMap.h"


#ifndef USE_MPI
#define USE_MPI false
#endif

#if USE_MPI
#include <mpi.h>
#endif

namespace capi {

using namespace llvm;

// Modified from XRay's func-id-helper.h
struct FuncIdConversionHelper {
public:
  using FunctionAddressMap = std::unordered_map<int32_t, uint64_t>;

private:
  std::string BinaryInstrMap;
  symbolize::LLVMSymbolizer &Symbolizer;
  const FunctionAddressMap &FunctionAddresses;
  mutable llvm::DenseMap<int32_t, std::string> CachedNames;

public:
  FuncIdConversionHelper(std::string BinaryInstrMap,
                         symbolize::LLVMSymbolizer &Symbolizer,
                         const FunctionAddressMap &FunctionAddresses)
      : BinaryInstrMap(std::move(BinaryInstrMap)), Symbolizer(Symbolizer),
        FunctionAddresses(FunctionAddresses) {}

  // Returns the symbol or a string representation of the function id.
  std::string getSymbol(int32_t FuncId) const {
    auto CacheIt = CachedNames.find(FuncId);
    if (CacheIt != CachedNames.end())
      return CacheIt->second;

    std::string sym{};
    auto It = FunctionAddresses.find(FuncId);
    if (It == FunctionAddresses.end()) {
      return {};
    }

    object::SectionedAddress ModuleAddress;
    ModuleAddress.Address = It->second;
    // TODO: set proper section index here.
    // object::SectionedAddress::UndefSection works for only absolute addresses.
    ModuleAddress.SectionIndex = object::SectionedAddress::UndefSection;
    if (auto ResOrErr = Symbolizer.symbolizeCode(BinaryInstrMap, ModuleAddress)) {
      auto &DI = *ResOrErr;
      if (DI.FunctionName != DILineInfo::BadString)
        sym = DI.FunctionName;

    } else
      handleAllErrors(ResOrErr.takeError(), [&](const ErrorInfoBase &) {
        logError() << "Symbolizer error\n"; // TODO: More information
      });

    CachedNames[FuncId] = sym;
    return sym;
  }

};


std::unordered_map<int, XRayFunctionInfo> loadXRayIDs(std::string& objectFile) {
  // It would be cleaner to use the XRay API directly.
  // However, this would require that the target application links against the static LLVM libraries itself, which makes things messy...
  //auto cmdStr = "llvm-xray extract --symbolize --no-demangle " + objectFile;

  std::unordered_map<int, XRayFunctionInfo> xrayIdMap;

  llvm::Expected<llvm::xray::InstrumentationMap> mapOrErr = llvm::xray::loadInstrumentationMap(objectFile);
  if (!mapOrErr) {
    logError() << "Unable to load XRay instrumentation map\n";
    return xrayIdMap;
  }
  auto& instrMap = mapOrErr.get();

  llvm::symbolize::LLVMSymbolizer::Options opts;
  opts.Demangle = false;
  llvm::symbolize::LLVMSymbolizer symbolizer(opts);

  const auto &funcAddresses = instrMap.getFunctionAddresses();

  FuncIdConversionHelper conversionHelper(objectFile, symbolizer, funcAddresses);

  auto& sleds = instrMap.sleds();

  int lastId = -1;
  for (const auto &sled : sleds) {
    auto fid = instrMap.getFunctionId(sled.Function);

    // Process each function ID only once
    if (!fid || *fid == lastId)
      continue;

    xrayIdMap[*fid] = {*fid, conversionHelper.getSymbol(*fid), sled.Function};

  }

  return xrayIdMap;

}

extern void handleXRayEvent(int32_t id, XRayEntryType type);

extern void postXRayInit(const XRayFunctionMap &);

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
    return;
  }

  auto execPath = getExecPath();
  auto execFilename = execPath.substr(execPath.find_last_of('/') + 1);

  auto symTables = loadMappedSymTables(execPath);

//  logInfo() << "Loading XRay IDs...\n";
//  loadXRayIDs(execPath);

  size_t numFound = 0;
  //size_t numInserted = 0;
  size_t numPatched = 0;
  size_t numFailed = 0;

  SymbolTable globalTable;
  std::unordered_set<uintptr_t> filteredOut;

  size_t numObjects = __xray_num_objects();

  XRayFunctionMap xrayMap;

//  for (auto&& [startAddr, table] : symTables) {
//    for (auto&& [addr, symName] : table.table) {
//      auto addrInProc = mapAddrToProc(addr, table);
//      if (noFilter || filter.accepts(symName)) {
//        globalTable[addrInProc] = symName;
//        ++numInserted;
//      } else {
//        filteredOut.insert(addrInProc);
//      }
//      ++numFound;
//    }
//  }

  __xray_init();
  __xray_set_handler(handleXRayEvent);

  for (int objId = 0; objId < numObjects; ++objId) {
    size_t maxFID = __xray_max_function_id_in_object(objId);
    if (maxFID == 0) {
      logError() << "No functions instrumented.\n";
      return;
    }

    // A somewhat hacky way to find out to which DSO this ID belongs
    std::string objName;
    uintptr_t firstAddr = __xray_function_address_in_object(1, objId);
    auto nextHighestIt = symTables.upper_bound(firstAddr);
    if (nextHighestIt == symTables.begin()) {
      logInfo() << "Unable to detect DSO name\n";
    } else {
      nextHighestIt--;
      objName = nextHighestIt->second.memMap.path;
    }

    logInfo() << "Detected " << maxFID << " patchable functions in object " << objId << " (" << objName << ")\n";

    auto funcInfoMap = loadXRayIDs(objName);

    numFound += funcInfoMap.size();

    for (int fid = 1; fid <= maxFID; ++fid) {
      //uintptr_t addr = __xray_function_address_in_object(fid, objId);
//      if (!addr) {
//        logError() << "Unable to determine address for function " << fid << "\n";
//        continue;
//      }
//      if (filteredOut.find(addr) != filteredOut.end()) {
//        continue;
//      }
      auto fIt = funcInfoMap.find(fid);
      if (fIt == funcInfoMap.end()) {
        logError() << "Unable to determine symbol for function " << fid << "\n";
        continue;
      }
      auto& fInfo = fIt->second;
      if (!filter.accepts(fInfo.name)) {
        continue;
      }

      auto patchStatus = __xray_patch_function_in_object(fid, objId);
      if (patchStatus == SUCCESS) {
        //logInfo() << "Patched function " << std::hex << addr << std::dec << ": id=" << fid << ", name=" << it->second << "\n";
        xrayMap[__xray_pack_id(fid, objId)] = fInfo;
        numPatched++;
      } else {
        logError() << "XRay patching failed: object=" << objId << ", fid=" << fid << ", name=" << fInfo.name << "\n";
        numFailed++;
      }

//      if (auto it = globalTable.find(addr); it != globalTable.end()) {
//        auto patchStatus = __xray_patch_function_in_object(fid, objId);
//        if (patchStatus == SUCCESS) {
//          //logInfo() << "Patched function " << std::hex << addr << std::dec << ": id=" << fid << ", name=" << it->second << "\n";
//          numPatched++;
//        } else {
//          logError() << "XRay patching at " << std::hex << addr << std::dec << " failed: object=" << objId << ", fid=" << fid << ", name=" << it->second << "\n";
//          numFailed++;
//        }
//      } else {
//        logError() << "Unable to find symbol for patchable function: object=" << objId << ", fid=" << fid << ", addr=" << std::hex << addr << std::dec << "\n";
//        numFailed++;
//      }
    }
  }

  logInfo() << "Functions found: " << numFound << "\n";
  //logInfo() << "Functions registered: " << numInserted << "\n";
  logInfo() << "Functions patched: " << numPatched << " (" << numFailed << " failed)\n";

  postXRayInit(xrayMap);

}

}

namespace {
  struct InitXRay {
    InitXRay() { capi::initXRay(); }
  };

  InitXRay _;
}
