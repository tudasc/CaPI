//
// Created by sebastian on 29.06.23.
//

#ifndef CAPI_CONTEXTSELECTORSCC_H
#define CAPI_CONTEXTSELECTORSCC_H

#include "Selector.h"

namespace capi {



class ContextSelectorSCC : public Selector {
public:
  enum CAHeuristicType {
    ALL, PARTIALLY_DISTINCT, DISTINCT
  };
private:

  CallGraph *cg{nullptr};
  int maxLCADist;
  CAHeuristicType type;

public:

  explicit ContextSelectorSCC(int maxLCADist, CAHeuristicType type) : maxLCADist(maxLCADist), type(type) {};

  void init(CallGraph &cg) override {
    this->cg = &cg;
  }

  FunctionSet apply(const FunctionSetList& input) override;

  std::string getName() override {
   return "ContextSelector2";
  }


};

}


#endif // CAPI_CONTEXTSELECTORSCC_H
