//
// Created by sebastian on 29.06.23.
//

#ifndef CAPI_COMMONCALLERSELECTORSCC_H
#define CAPI_COMMONCALLERSELECTORSCC_H

#include "Selector.h"

namespace capi {



class CommonCallerSelectorSCC : public Selector {
public:
  enum CAHeuristicType {
    ALL, PARTIALLY_DISTINCT, DISTINCT
  };
private:

  TraversalHelper *helper{nullptr};
  int maxLCADist;
  CAHeuristicType type;

public:

  explicit CommonCallerSelectorSCC(int maxLCADist, CAHeuristicType type) : maxLCADist(maxLCADist), type(type) {};

  void init(TraversalHelper &helper) override {
    this->helper = &helper;
  }

  FunctionSet apply(const FunctionSetList& input) override;

  std::string getName() override {
   return "CommonCallerSelector";
  }


};



}


#endif // CAPI_COMMONCALLERSELECTORSCC_H
