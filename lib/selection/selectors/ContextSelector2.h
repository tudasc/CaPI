//
// Created by sebastian on 29.06.23.
//

#ifndef CAPI_CONTEXTSELECTOR2_H
#define CAPI_CONTEXTSELECTOR2_H

#include "Selector.h"

namespace capi {

class ContextSelector2 : public Selector {
  CallGraph *cg{nullptr};

public:
  ContextSelector2() = default;

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
