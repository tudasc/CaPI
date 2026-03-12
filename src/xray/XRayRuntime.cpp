//
// Created by sebastian on 21.03.22.
//

#include "capi_version.h"
#include "XRayRuntime.h"
#include "capi/support/Logging.h"
#include "capi/support/Timer.h"
#include "capi/symbol_retriever/SymbolRetriever.h"
#include "capi/selection/FunctionFilter.h"
#include "capi/selection/MeasurementConfig.h"
#include "capi/selection/MeasurementConfigIO.h"
#include "CallLogger.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <link.h>
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

CAPI_DEFINE_VERBOSITY(LOG_STATUS)

namespace capi {

static std::atomic_bool runtimeInitialized{false};
static std::atomic_bool runtimeActive{false};
static int numInitialObjects = 0;
static std::vector<const char*>* graphsToMerge{nullptr};
static bool ignoreIndirect;

XRayMeasurementConfig::XRayMeasurementConfig(const capi::MeasurementConfig& mc, const XRayFunctionMap& xrayMap) {
  std::unordered_set<std::string> enteredFunctions;
  for (const auto& [id, info] : xrayMap) {
    auto* entries = mc.get(info.name);
    if (!entries) {
      logError() << "Could not find any measurement config entries for instrumented function '" << info.name << "'.\n";
      continue;
    }
    // Copy all entries into id map.
    pathEntries[id] = *entries;
    enteredFunctions.insert(info.name);
  }
  // Checking if we got all entries
  int numMissing = 0;
  for (const auto& [fname, entries] : mc.entries()) {
    if (!enteredFunctions.contains(fname)) {
      numMissing++;
    }
  }
  if (numMissing > 0) {
    logError() << numMissing << " functions from measurement config not instrumented.\n";
  }
}

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

XRayFunctionMap loadXRayIDs(std::string& objectFile) XRAY_NEVER_INSTRUMENT {
  XRayFunctionMap xrayIdMap;

  llvm::Expected<llvm::xray::InstrumentationMap> mapOrErr = llvm::xray::loadInstrumentationMap(objectFile);
  if (auto E = mapOrErr.takeError()) {
    logError() << "Unable to load XRay instrumentation map: " << toString(std::move(E)) << "\n";
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
    auto demangledName = llvm::demangle(name);

    xrayIdMap[*fid] = {*fid, name, demangledName,  sled.Function};

  }

  return xrayIdMap;

}

// Stored behind a pointer to avoid initialization order problems.
capi::GlobalCaPIData* globalCaPIData;


extern void handleXRayEvent(int32_t id, XRayEntryType type);

extern void handleCustomXRayEvent(void* data, size_t len);

extern void postXRayInit();

extern void preXRayFinalize();

std::vector<std::string> splitArgs(const std::string& input) {
  std::istringstream iss(input);
  std::vector<std::string> args;
  std::string token;
  while (iss >> token) {
    args.push_back(token);
  }
  return args;
}

cxxopts::ParseResult parseOptions() {

  std::vector<std::string> args;
  const char* env = std::getenv("CAPI_OPTIONS");
  if (env && !std::string(env).empty()) {
    args = splitArgs(env);
  }
  args.insert(args.begin(), "capi-runtime"); // argv[0] dummy

  std::vector<const char*> argv;
  argv.reserve(args.size());
  for (const auto& s : args) {
    argv.push_back(s.c_str());
  }

  cxxopts::Options options("capi-options", "CaPI runtime common options");


  options.add_options()
      ("enable", "Enable instrumentation",
       cxxopts::value<bool>()->default_value("false"))
      ("log-calls", "Log instrumented calls",
       cxxopts::value<bool>()->default_value("false"))
      ("ignore-indirect-calls", "Disable indirect call handling",
       cxxopts::value<bool>()->default_value("false"))
      ("config", "Measurement configuration file",
       cxxopts::value<std::string>()->default_value(""))
      ("filter-file", "Filter file (deprecated, use --config instead)",
       cxxopts::value<std::string>()->default_value(""));

  capi::registerExtraOptions(options);
  auto result = options.parse(static_cast<int>(args.size()), argv.data());
  return result;

}

struct PatchingStats {
  int numFound{0};
  int numPatched{0};
  int numFailed{0};
};

static std::string detectObject(int objId, MappedSymTableMap& symTables) {
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
  return objName;
}


static PatchingStats patchObject(int objId, const XRayFunctionMap& localMap, XRayFunctionMap& globalMap, FunctionFilter* filter, Timer* patchTimer) {

  size_t maxFID = __xray_max_function_id_in_object(objId);
  if (maxFID == 0) {
    logError() << "Detected no XRay sleds - no functions instrumented.\n";
    return {};
  }

  PatchingStats stats;

  stats.numFound += localMap.size();

  if (patchTimer)
    patchTimer->resume();
  for (int fid = 1; fid <= maxFID; ++fid) {
    auto fIt = localMap.find(fid);
    if (fIt == localMap.end()) {
      logError() << "Unable to determine symbol for function " << fid << "\n";
      continue;
    }
    auto& fInfo = fIt->second;

    // Ignore if there is no filter or the function is filtered out
    if (filter && !filter->accepts(fInfo.name)) {
      continue;
    }

    auto patchStatus = __xray_patch_function_in_object(fid, objId);
    if (patchStatus == SUCCESS) {
      auto packedId = __xray_pack_id(fid, objId);
      globalMap[packedId] = fInfo;

      if (filter) {
        // TODO: This should not access global data directly
        int flags = filter->getFlags(fInfo.name);
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
      }
      stats.numPatched++;
    } else {
      logError() << "XRay patching failed: object=" << objId << ", fid=" << fid << ", name=" << fInfo.name << "\n";
      stats.numFailed++;
    }
  }
  if (patchTimer)
    patchTimer->pause();
  return stats;
}


static PatchingStats loadIdsAndPatchObject(int objId, std::string objName, XRayFunctionMap& globalMap, FunctionFilter* filter, Timer* idLoadTimer, Timer* patchTimer) {

  if (idLoadTimer)
    idLoadTimer->resume();
  auto funcInfoMap = loadXRayIDs(objName);
  if (idLoadTimer)
    idLoadTimer->pause();

  size_t maxFID = __xray_max_function_id_in_object(objId);

  logInfo() << "Detected " << maxFID << " patchable functions in object " << objId << " (" << objName << ")" << std::endl;

  return patchObject(objId, funcInfoMap, globalMap, filter, patchTimer);
}


void initXRay() XRAY_NEVER_INSTRUMENT {

    bool expected = false;
    if (!runtimeInitialized.compare_exchange_strong(
            expected, true,
            std::memory_order_acq_rel)) {
        return;
    }

  logInfo() << "Running with DynCaPI Version " << CAPI_VERSION_MAJOR << "." << CAPI_VERSION_MINOR << std::endl;
  logInfo() << "Git revision: " << CAPI_GIT_SHA1 << std::endl;

  Timer timer("[Info] Full initialization took ", std::cout);

  bool shouldInit{false};
  bool logCalls{false};

  std::unique_ptr<FunctionFilter> filter;
  std::unique_ptr<MeasurementConfig> mc;

  auto result = parseOptions();

  logCalls = result["log-calls"].as<bool>();
  ignoreIndirect = result["ignore-indirect-calls"].as<bool>();
  std::string mcFile = result["config"].as<std::string>();
  std::string filterFile = result["filter-file"].as<std::string>();

  shouldInit = result["enable"].as<bool>();

  if (!mcFile.empty()) {
    logInfo() << "Loading measurement config from " << mcFile << "...\n";
    mc = read(mcFile);
    if (mc) {
      filter = std::make_unique<FunctionFilter>(mc->createFunctionFilter());
      shouldInit = true;
    }
  } else if (!filterFile.empty()) {
    Timer timer("[Info] Loading filter file took ", std::cout);
    filter = std::make_unique<FunctionFilter>();
    bool success{false};
    if (filterFile.ends_with(".json")) {
      success = readJSONFilterFile(*filter, filterFile);
    } else {
      success = readScorePFilterFile(*filter, filterFile);
    }
    if (success) {
      logInfo() << "Loaded filter file with " << filter->size() << " entries.\n";
      shouldInit = true;
    } else {
      filter.reset();
      logError() << "Failed to read filter file from " << filterFile << "\n";
      return;
    }
  } else {
    logInfo() << "No CaPI filtering file specified.\n";
  }


  if (!shouldInit) {
    logInfo() << "CaPI is inactive. Set '--config <config_file>' or '--enable' in 'CAPI_OPTIONS' if you want to activate instrumentation.\n";
    return;
  }

  globalCaPIData = new GlobalCaPIData;

  auto execPath = getExecPath();
  auto execFilename = execPath.substr(execPath.find_last_of('/') + 1);

  MappedSymTableMap symTables;
  {
    Timer timer("[Info] Reading symtables took ", std::cout);
    symTables = loadMappedSymTables(execPath, false);
  }

  std::unordered_set<uintptr_t> filteredOut;

  size_t numObjects = __xray_num_objects();
  numObjects = std::min(__xray_num_objects(), 255ul); // FIXME: Workaround for bug in XRay

  numInitialObjects = numObjects;

  XRayFunctionMap xrayMap;

  __xray_init();
  __xray_set_handler(handleXRayEvent);
  __xray_set_customevent_handler(handleCustomXRayEvent);

  PatchingStats fullStats;

  {
    Timer idLoadTimer("[Info] Loading IDs took ", std::cout, false);
    Timer patchTimer("[Info] Patching took ", std::cout, false);
    for (int objId = 0; objId < numObjects; ++objId) {
      auto objName = detectObject(objId, symTables);
      auto objectStats = loadIdsAndPatchObject(objId, objName, xrayMap, filter.get(), &idLoadTimer, &patchTimer);
      fullStats.numFound += objectStats.numFound;
      fullStats.numPatched += objectStats.numPatched;
      fullStats.numFailed += objectStats.numFailed;
    }
  }

  std::unique_ptr<XRayMeasurementConfig> xmc;
  if (mc) {
    xmc = std::make_unique<XRayMeasurementConfig>(*mc, xrayMap);
  }

  globalCaPIData->options = result;
  globalCaPIData->xrayFuncMap = std::move(xrayMap);
  globalCaPIData->filter = std::move(filter);
  globalCaPIData->xrayMeasurementConfig = std::move(xmc);
  globalCaPIData->measurementConfig = std::move(mc);
  globalCaPIData->symTables = std::move(symTables);
  globalCaPIData->useScopeTriggers = !globalCaPIData->scopeTriggerSet.empty();
  globalCaPIData->logCalls = logCalls;
  if (logCalls) {
    logInfo() << "Call logging is active\n";
    globalCaPIData->logger = std::make_unique<CallLogger>(execFilename);
  }
    auto mainGraph = extractMainGraph();
    if (!mainGraph) {
        logWarn() << "Could not load embedded graph - running without runtime graph\n";
    }

    // Load and merge DSO call graphs
    if (mainGraph && capi::graphsToMerge) {
        for (auto rawCg : *graphsToMerge) {
            if (rawCg) {
                auto dsoCg = capi::loadGraphFromStr(rawCg);
                if (dsoCg) {
                    mainGraph->merge(*dsoCg, cage::DynamicLinkagePolicy{});
                }
            }
        }
    }
    globalCaPIData->runtimeGraph = std::make_unique<RuntimeGraph>(std::move(mainGraph));

  logInfo() << "Functions found: " << fullStats.numFound << "\n";
  logInfo() << "Functions patched: " << fullStats.numPatched << " (" << fullStats.numFailed << " failed)\n";

  runtimeActive.store(true, std::memory_order_release);

  postXRayInit();
}


void finalizeXRay() XRAY_NEVER_INSTRUMENT {
  runtimeActive.store(false, std::memory_order_release);
  preXRayFinalize();
  delete globalCaPIData;
  globalCaPIData = nullptr;
}

}

extern "C" __attribute__((visibility("default"))) void capi_register_dso(uint64_t firstFunctionAddr, const char* rawCg) XRAY_NEVER_INSTRUMENT {

  if (!capi::runtimeActive.load(std::memory_order_acquire)) {
      // Store for loading during init
      if (!capi::graphsToMerge) {
          // Need pointer here to avoid static initialization order fiasco
          capi::graphsToMerge = new std::vector<const char*>();
      }
      capi::graphsToMerge->push_back(rawCg);
      capi::logInfo() << "Registered DSO! Graph stored for merging during init...\n";
      return;
  }


  // Patching only libraries that are loaded *after* the initial setup.
  // TODO: Should they also be merged into the runtime graph for validation? Probably not?

  size_t numObjects = std::min(__xray_num_objects(), 255ul); // FIXME: Workaround for bug in XRay
  for (int objId = capi::numInitialObjects; objId < numObjects; objId++) {
      if (__xray_max_function_id_in_object(objId) == 0) {
        continue;
      }
//      capi::logInfo() << "First adddress in " << i << " is " << std::hex <<  __xray_function_address_in_object(1, i) << ", target address is " << firstFunctionAddr << std::dec << "\n";
      if (__xray_function_address_in_object(1, objId) == firstFunctionAddr) {

        capi::logInfo() << "Intercepted loading of DSO with ID=" << objId << " with first function at address " << std::hex << firstFunctionAddr << std::dec << "\n";

        struct FindDsoCtx {
          uintptr_t target;
          const char* name = nullptr;
          uintptr_t addr;
        };

        FindDsoCtx ctx;
        ctx.target = firstFunctionAddr;
        dl_iterate_phdr([](struct dl_phdr_info* info, size_t, void* data) -> int {
          auto* ctx = static_cast<FindDsoCtx*>(data);
          for (int i = 0; i < info->dlpi_phnum; ++i) {
            const ElfW(Phdr)& ph = info->dlpi_phdr[i];

            if (ph.p_type != PT_LOAD)
              continue;

            uintptr_t start = info->dlpi_addr + ph.p_vaddr;
            uintptr_t end   = start + ph.p_memsz;

            //capi::logInfo() << "Checking segment: start=" << std::hex << start << ", end=" << end << std::dec << "\n";

            if (ctx->target >= start && ctx->target < end) {
              ctx->name = info->dlpi_name && info->dlpi_name[0]
                                ? info->dlpi_name
                                : "<main executable>";
              ctx->addr = start;
              return 1; // stop iteration
            }
          }
//          capi::logInfo() << "Detected loading of DSO " << (info->dlpi_name[0] ? info->dlpi_name : "unknown" ) << " at address "
//                                      << std::hex << (void*)info->dlpi_addr << std::dec << "\n";
          return 0;
        }, &ctx);

        if (!ctx.name) {
          capi::logError() << "Could not detect corresponding object file!\n";
          return;
        }

        capi::logInfo() << "Loading symbols and patching DSO " << ctx.name << "\n";

        auto symTable = loadSymbolTable(ctx.name);
        if (!symTable.empty()) {
            // FIXME: Adress mapping is not correct
            MappedSymTable mappedTable(std::move(symTable), MemMapEntry(ctx.name, ctx.addr, 0));
            capi::globalCaPIData->symTables[ctx.addr] = std::move(mappedTable);
        }

        auto& xrayMap = capi::globalCaPIData->xrayFuncMap;
        auto& filter = capi::globalCaPIData->filter;

        // TODO: Reenable patching of loaded objects
        //auto objectStats = loadIdsAndPatchObject(objId, ctx.name, xrayMap, filter.get(), nullptr, nullptr);
        //__xray_patch_object(objId);
      }

  }

}

static std::mutex rtGraphMutex;

extern "C" void __metacg_indirect_call(const char* name, void* address) XRAY_NEVER_INSTRUMENT {
    if (!capi::runtimeActive.load(std::memory_order_acquire) || !capi::globalCaPIData) {
        // CaPI was not initialized or is finalized
        return;
    }
    if (capi::ignoreIndirect) {
        return;
    }

    std::lock_guard<std::mutex> lock(rtGraphMutex);

    static std::unordered_map<const char*, std::unordered_set<void*>> visitedMap;

    auto& knownCalls = visitedMap[name];
    if (knownCalls.find(address) != knownCalls.end()) {
        // We have seen this edge before
        return;
    }
    knownCalls.insert(address);

    const std::string& symbol = findSymbol(reinterpret_cast<std::uintptr_t>(address), capi::globalCaPIData->symTables);
    if (symbol.empty()) {
        capi::logError() << "Could not resolve symbol for call to address " << std::hex << reinterpret_cast<std::uintptr_t>(address) << std::dec << " from " << name << "\n";
        return;
    }
    capi::globalCaPIData->runtimeGraph->recordIndirectCall(name, symbol);

}

namespace {
  struct InitXRay {
    InitXRay() XRAY_NEVER_INSTRUMENT { capi::initXRay(); }
    ~InitXRay() XRAY_NEVER_INSTRUMENT { capi::finalizeXRay(); }
  };

  InitXRay _;
}
