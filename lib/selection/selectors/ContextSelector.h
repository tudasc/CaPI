//
// Created by sebastian on 29.06.23.
//

#ifndef CAPI_CONTEXTSELECTOR_H
#define CAPI_CONTEXTSELECTOR_H

#include "Selector.h"

namespace capi {

class ContextSelector : public Selector {
  CallGraph *cg{nullptr};

public:
  ContextSelector() = default;

  void init(CallGraph &cg) override {
    this->cg = &cg;
  }

  FunctionSet apply(const FunctionSetList& input) override;

  std::string getName() override {
   return "ContextSelector";
  }
};

}


#endif // CAPI_CONTEXTSELECTOR_H
