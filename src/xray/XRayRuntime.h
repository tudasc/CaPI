//
// Created by sebastian on 21.03.22.
//

#ifndef CAPI_XRAYINTERFACE_H
#define CAPI_XRAYINTERFACE_H

#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <sstream>

#define CXXOPTS_NO_RTTI
#include "cxxopts.hpp"

#include "capi/selection/MeasurementConfig.h"
#include "capi/support/IteratorUtils.h"
#include "capi/symbol_retriever/SymbolRetriever.h"
#include "xray/xray_interface.h"
#include "RuntimeGraph.h"

#define XRAY_NEVER_INSTRUMENT __attribute__((xray_never_instrument))

namespace capi {

class CallLogger;

struct XRayFunctionInfo {
  int functionId{0};
  std::string name{};
  std::string demangled{};
  uint64_t addr{0};
};

using XRayFunctionMap = std::unordered_map<int, XRayFunctionInfo>;

struct XRayMeasurementConfig {
  XRayMeasurementConfig(const MeasurementConfig& mc, const XRayFunctionMap& xrayMap);

 private:
  std::unordered_map<int, PathEntries> pathEntries;
};

struct GlobalCaPIData {
  XRayFunctionMap xrayFuncMap;
  std::unordered_set<int32_t> scopeTriggerSet;
  std::unordered_set<int32_t> beginTriggerSet;
  std::unordered_set<int32_t> endTriggerSet;
  bool beginActive{true};
  bool useScopeTriggers{false};
  std::unique_ptr<FunctionFilter> filter;
  std::unique_ptr<MeasurementConfig> measurementConfig;
  std::unique_ptr<XRayMeasurementConfig> xrayMeasurementConfig;
  bool logCalls;
  std::unique_ptr<CallLogger> logger;
  cxxopts::ParseResult options;
  MappedSymTableMap symTables;
  std::unique_ptr<RuntimeGraph> runtimeGraph;
  std::unordered_set<std::string> finalDynamicFilterSet;
};

using XRayHandlerFn = void (*)(int32_t, XRayEntryType);

struct XRayRecursionGuard {
  bool& xrayScope;
  bool wasInScope;

  XRayRecursionGuard(bool& xrayScope) XRAY_NEVER_INSTRUMENT : xrayScope(xrayScope) {
    wasInScope = xrayScope;
    xrayScope = true;
  }

  ~XRayRecursionGuard() XRAY_NEVER_INSTRUMENT {
    xrayScope = false;
  }

  bool check() const XRAY_NEVER_INSTRUMENT {
    return !wasInScope;
  }

  operator bool() const XRAY_NEVER_INSTRUMENT {
    return check();
  }
};

void registerExtraOptions(cxxopts::Options&);

void initXRay();

}

#endif // CAPI_XRAYINTERFACE_H
