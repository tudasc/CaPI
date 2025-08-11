//
// Created by sebastian on 11.08.25.
//

#include "capi/selection/MeasurementConfig.h"
#include "capi/selection/FunctionFilter.h"

namespace capi {

FunctionFilter MeasurementConfig::createFunctionFilter() const {
  FunctionFilter filter;
  for (auto& [name, pathEntries] : selectedFunctions) {
    // Note: No support for flags here
    filter.addIncludedFunction(name);
  }
  return filter;
}

}