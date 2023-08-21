//
// Created by sebastian on 29.06.23.
//

#ifndef CAPI_CONTEXTSELECTOR2_H
#define CAPI_CONTEXTSELECTOR2_H

#include "Selector.h"

namespace capi {

enum class CAHeuristicType {
  ALL, PARTIALLY_DISTINCT, DISTINCT
};

class ContextSelector2 : public Selector {
  CallGraph *cg{nullptr};
  int maxLCADist;
  CAHeuristicType type;

public:
  explicit ContextSelector2(int maxLCADist, CAHeuristicType type) : maxLCADist(maxLCADist), type(type) {};

  void init(CallGraph &cg) override {
    this->cg = &cg;
  }

  FunctionSet apply(const FunctionSetList& input) override;

  std::string getName() override {
   return "ContextSelector2";
  }
};

}


#endif // CAPI_CONTEXTSELECTOR2_H
