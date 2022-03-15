//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_SELECTORBUILDER_H
#define CAPI_SELECTORBUILDER_H

#include "SelectionSpecAST.h"
#include "SelectorGraph.h"
#include "Selectors.h"
#include <cassert>
#include <variant>


namespace capi {

struct Param {

  using ParamVal = std::variant<bool, int, float, std::string>;

  enum Kind {
    INT, FLOAT, BOOL, STRING
  };


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


  //    SelectorPtr selector;
  //    CGNode* node;

private:
  Param() = default;
  Param(Kind kind, ParamVal val) : kind(kind), val(val){}
};

using SelectorFactoryFn = std::function<SelectorPtr(std::vector<Param>)>;



struct RegisterSelector {
  RegisterSelector(std::string selectorType, SelectorFactoryFn fn);
};



SelectorGraphPtr buildSelectorGraph(SpecAST& ast);

}

#endif // CAPI_SELECTORBUILDER_H
