//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_SELECTORREGISTRY_H
#define CAPI_SELECTORREGISTRY_H

#include <variant>

#include "capi/selection/Selector.h"

namespace capi {

using SelectorFactoryFn = std::function<SelectorPtr(const std::vector < Param > &)>;

struct RegisterSelector
{
  RegisterSelector(std::string selectorType, SelectorFactoryFn fn);
};

}

#endif //CAPI_SELECTORREGISTRY_H
