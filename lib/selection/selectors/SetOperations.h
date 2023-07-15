//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_SETOPERATIONS_H
#define CAPI_SETOPERATIONS_H

#include "Selector.h"

namespace capi {


enum class SetOperation { UNION, INTERSECTION, COMPLEMENT };

template <SetOperation Op> class SetOperationSelector : public Selector {
public:
  SetOperationSelector() = default;

  void init(CallGraph &cg) override {
  }

  FunctionSet apply(const FunctionSetList& input) override;

  std::string getName() override {
    switch(Op) {
      case SetOperation::UNION:
        return "join";
      case SetOperation::INTERSECTION:
        return "intersect";
      case SetOperation::COMPLEMENT:
        return "subtract";
    }
    return "invalid";
  }
};


template <SetOperation Op> FunctionSet SetOperationSelector<Op>::apply(const FunctionSetList& input) {
  static_assert(Op == SetOperation::UNION || Op == SetOperation::INTERSECTION ||
                Op == SetOperation::COMPLEMENT);

  if (input.size() != 2) {
    logError() << "Expected exactly two input sets, got " << input.size() << " instead.\n";
    return {};
  }

  FunctionSet inA = input[0];
  FunctionSet inB = input[1];

  FunctionSet out;

  if constexpr (Op == SetOperation::UNION) {
    out.insert(inA.begin(), inA.end());
    // Check for duplicates. This would be easier using a set, not sure about performance.
    for (auto& fB : inB) {
      if (inA.find(fB) == inA.end()) {
        out.insert(fB);
      }
    }
  } else if constexpr (Op == SetOperation::INTERSECTION) {
    for (auto &fA : inA) {
      if (inB.find(fA) != inB.end()) {
        out.insert(fA);
      }
    }
  } else if constexpr (Op == SetOperation::COMPLEMENT) {
    for (auto &fA : inA) {
      if (inB.find(fA) == inB.end()) {
        out.insert(fA);
      }
    }
  }
  return out;
}


}


#endif //CAPI_SETOPERATIONS_H
