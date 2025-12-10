//
// Created by sebastian on 01.04.25.
//

#ifndef CAPI_DEMANGLEMD_H
#define CAPI_DEMANGLEMD_H

#include "capi/selection/metadata/TransientMD.h"

namespace capi {

struct FunctionInfo {
  std::string demangledName;
  std::vector<std::string> parameters;
  bool isTrigger{false};
};


struct CaPIMDKey {
  static constexpr const char* key = "capi";
};
using CaPIMD = TransientMD<FunctionInfo, CaPIMDKey>;

}

#endif // CAPI_DEMANGLEMD_H
