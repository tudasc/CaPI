//
// Created by sebastian on 21.03.22.
//

#include "capi_version.h"
#include "XRayRuntime.h"
#include "../Utils.h"
#include "../selection/FunctionFilter.h"
#include "SymbolRetriever.h"
#include "CallLogger.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <unordered_set>

#include "xray/xray_interface.h"

#include "llvm/XRay/InstrumentationMap.h"
#include "llvm/DebugInfo/Symbolize/Symbolize.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Demangle/Demangle.h"

#ifdef WITH_MPI
#include <mpi.h>
#endif

CAPI_DEFINE_VERBOSITY

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
                         const FunctionAddressMap &FunctionAddresses) XRAY_NEVER_INSTRUMENT
      : BinaryInstrMap(std::move(BinaryInstrMap)), Symbolizer(Symbolizer),
        FunctionAddresses(FunctionAddresses) {}

  // Returns the symbol or a string representation of the function id.
  std::string getSymbol(int32_t FuncId) const XRAY_NEVER_INSTRUMENT {
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


std::unordered_map<int, XRayFunctionInfo> loadXRayIDs(std::string& objectFile) XRAY_NEVER_INSTRUMENT {
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

    auto name = conversionHelper.getSymbol(*fid);
    auto demangledName = name;

    int status = 0;
    char* demangleResult = llvm::itaniumDemangle(name.c_str(), nullptr, nullptr, &status);
    if (status == 0) {
      demangledName = demangleResult;
      free(demangleResult);
    }

    xrayIdMap[*fid] = {*fid, name, demangledName,  sled.Function};

  }

  return xrayIdMap;

}

// Stored behind a pointer to avoid initialization order problems.
capi::GlobalCaPIData* globalCaPIData;

extern void handleXRayEvent(int32_t id, XRayEntryType type);

extern void postXRayInit(const XRayFunctionMap &);

extern void preXRayFinalize();

void initXRay() XRAY_NEVER_INSTRUMENT {
  logInfo() << "Running with DynCaPI Version " << CAPI_VERSION_MAJOR << "." << CAPI_VERSION_MINOR << "\n";
  logInfo() << "Git revision: " << CAPI_GIT_SHA1 << "\n";

  Timer timer("[Info] Initialization took ", std::cout);

  bool shouldInit{false};
  bool logCalls{false};

  auto enableEnv = std::getenv("CAPI_ENABLE");
  if (enableEnv) {
    shouldInit = true;
  }

  auto logCallsEnv = std::getenv("CAPI_LOG_CALLS");
  if (logCallsEnv) {
    logCalls = true;
  }


  bool noFilter{true};
  FunctionFilter filter;
  auto filterEnv = std::getenv("CAPI_FILTERING_FILE");
  if (filterEnv) {
    bool success{false};
    if (0 == strncmp( filterEnv + strlen(filterEnv) - 5, ".json", 5)) {
      success = readJSONFilterFile(filter, filterEnv);
    } else {
      success = readScorePFilterFile(filter, filterEnv);
    }
    if (success) {
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

  globalCaPIData = new GlobalCaPIData;

  auto execPath = getExecPath();
  auto execFilename = execPath.substr(execPath.find_last_of('/') + 1);

  auto symTables = loadMappedSymTables(execPath);

  size_t numFound = 0;
  size_t numPatched = 0;
  size_t numFailed = 0;

  SymbolTable globalTable;
  std::unordered_set<uintptr_t> filteredOut;

  size_t numObjects = __xray_num_objects();

  XRayFunctionMap xrayMap;

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

      auto fIt = funcInfoMap.find(fid);
      if (fIt == funcInfoMap.end()) {
        logError() << "Unable to determine symbol for function " << fid << "\n";
        continue;
      }
      auto& fInfo = fIt->second;
      if (!(noFilter || filter.accepts(fInfo.name))) {
        continue;
      }

      auto patchStatus = __xray_patch_function_in_object(fid, objId);
      if (patchStatus == SUCCESS) {
        auto packedId = __xray_pack_id(fid, objId);
        xrayMap[packedId] = fInfo;
        int flags = filter.getFlags(fInfo.name);
        if (isScopeTrigger(flags)) {
          globalCaPIData->scopeTriggerSet.insert(packedId);
        }
        if (isBeginTrigger(flags)) {
          globalCaPIData->beginTriggerSet.insert(packedId);
          // If there are begin triggers, start in inactive mode
          globalCaPIData->beginActive = false;
        }
        if (isEndTrigger(flags)) {
          globalCaPIData->endTriggerSet.insert(packedId);
        }
        numPatched++;
      } else {
        logError() << "XRay patching failed: object=" << objId << ", fid=" << fid << ", name=" << fInfo.name << "\n";
        numFailed++;
      }

    }
  }

  globalCaPIData->xrayFuncMap = xrayMap;
  globalCaPIData->useScopeTriggers = !globalCaPIData->scopeTriggerSet.empty();
  globalCaPIData->logCalls = logCalls;
  if (logCalls) {
    logInfo() << "Call logging is active\n";
    globalCaPIData->logger = std::make_unique<CallLogger>(execFilename);
  }

  logInfo() << "Functions found: " << numFound << "\n";
  logInfo() << "Functions patched: " << numPatched << " (" << numFailed << " failed)\n";


  // TODO: Since the map is now in the global data, there is no need to pass it.
  postXRayInit(xrayMap);

}

void finalizeXRay() XRAY_NEVER_INSTRUMENT {
  preXRayFinalize();
  delete globalCaPIData;
}


}

namespace {
  struct InitXRay {
    InitXRay() XRAY_NEVER_INSTRUMENT { capi::initXRay(); }
    ~InitXRay() XRAY_NEVER_INSTRUMENT { capi::finalizeXRay(); }
  };

  InitXRay _;
}
