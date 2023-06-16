//
// Created by sebastian on 21.03.22.
//

#ifndef CAPI_XRAYINTERFACE_H
#define CAPI_XRAYINTERFACE_H

#include <string>
#include <map>
#include <unordered_map>
#include <vector>

#include "xray/xray_interface.h"

#define XRAY_NEVER_INSTRUMENT __attribute__((xray_never_instrument))


namespace capi {

using XRayHandlerFn = void (*)(int32_t, XRayEntryType);

struct XRayFunctionInfo {
  int functionId{0};
  std::string name{};
  std::string demangled{};
  uint64_t addr{0};
};

using XRayFunctionMap = std::unordered_map<int, XRayFunctionInfo>;

void initXRay();

}

#endif // CAPI_XRAYINTERFACE_H
