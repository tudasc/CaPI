//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_SELECTORREGISTRY_H
#define CAPI_SELECTORREGISTRY_H

#include <variant>

#include "Selector.h"

namespace capi {

struct Param {

  using ParamVal = std::variant<bool, int, float, std::string>;

  enum Kind {
    INT, FLOAT, BOOL, STRING
  };

  const char* kindNames[4] = {"INT", "FLOAT", "BOOL", "STRING"};

   Kind kind;

  ParamVal val;

  static Param makeInt(int val) {
    return {INT, val};
  }

  static Param makeFloat(float val) {
    return {FLOAT, val};
  }

  static Param makeBool(bool val) {
    return {BOOL, val};
  }

  static Param makeString(std::string val) {
    return {STRING, val};
  }

private:
  Param() = default;
  Param(Kind kind, ParamVal val) : kind(kind), val(val){}
};

using SelectorFactoryFn = std::function<SelectorPtr(const std::vector < Param > &)>;

struct RegisterSelector
{
  RegisterSelector(std::string selectorType, SelectorFactoryFn fn);
};

}

#endif //CAPI_SELECTORREGISTRY_H
