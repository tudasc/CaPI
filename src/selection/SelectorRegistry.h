//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_SELECTORREGISTRY_H
#define CAPI_SELECTORREGISTRY_H

#include <variant>
#include <vector>

#include "capi/selection/Selector.h"

namespace capi {

using SelectorFactoryFn = std::function<SelectorPtr(const std::vector < Param > &)>;

enum class SelectorType {
  DEFAULT,
  TALP
};

struct SelectorDoc {
  std::string name;
  SelectorType type;
  std::vector<std::string> parameterLabels;
  std::vector<std::string> parameterTypes;
  std::string example;
  std::string description;
  std::string numInputs;

  SelectorDoc(const std::string& n, SelectorType t, const std::vector<std::string>& pLabels,
              const std::vector<std::string>& pTypes, const std::string& ex, const std::string& desc, const std::string& numInputs)
      : name(n), type(t), parameterLabels(pLabels), parameterTypes(pTypes), example(ex), description(desc), numInputs(numInputs)  {}
};

struct SelectorInfo {
  SelectorFactoryFn fn;
  std::optional<SelectorDoc> doc;
};

struct RegisterSelector
{
  RegisterSelector(std::string selectorType, SelectorFactoryFn fn, std::optional<SelectorDoc> doc = std::nullopt);
};

}

#endif //CAPI_SELECTORREGISTRY_H
