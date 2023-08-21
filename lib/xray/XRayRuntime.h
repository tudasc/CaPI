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

#include "xray/xray_interface.h"

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

struct GlobalCaPIData {
  XRayFunctionMap xrayFuncMap;
  std::unordered_set<int32_t> scopeTriggerSet;
  std::unordered_set<int32_t> beginTriggerSet;
  std::unordered_set<int32_t> endTriggerSet;
  bool beginActive{true};
  bool useScopeTriggers{false};
  bool logCalls;
  std::unique_ptr<CallLogger> logger;
};

using XRayHandlerFn = void (*)(int32_t, XRayEntryType);



void initXRay();

}

#endif // CAPI_XRAYINTERFACE_H
