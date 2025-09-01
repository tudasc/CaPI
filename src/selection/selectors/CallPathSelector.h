//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_CALLPATHSELECTOR_H
#define CAPI_CALLPATHSELECTOR_H

#include "capi/selection/Selector.h"

namespace capi {

enum class TraverseDir { TraverseUp, TraverseDown };

template <TraverseDir dir> class CallPathSelector : public Selector {
  TraversalHelper *helper{nullptr};

public:
  CallPathSelector() = default;

  void init(TraversalHelper &helper) override {
    this->helper = &helper;
  }

  FunctionSet apply(const FunctionSetList& input) override;

  std::string getName() override {
    if constexpr (dir == TraverseDir::TraverseUp) {
      return "CallPathSelector<TraverseUp>";
    }
    return "CallPathSelector<TraverseDown>";
  }
};

template <TraverseDir Dir> FunctionSet CallPathSelector<Dir>::apply(const FunctionSetList& input) {
  static_assert(Dir == TraverseDir::TraverseDown ||
                Dir == TraverseDir::TraverseUp);

  if (input.size() != 1) {
    logError() << "Expected exactly one input sets, got " << input.size() << " instead.\n";
    return {};
  }


  FunctionSet in = input.front();
  FunctionSet out(in);

  auto visitFn = [&out](const metacg::CgNode &node) {
    if (out.find(&node) == out.end()) {
      out.insert(&node);
    }
  };

  for (auto &fn : in) {
    if constexpr (Dir == TraverseDir::TraverseDown) {
      int count = traverseCallGraph(
              *fn, [this](const metacg::CgNode & node) -> auto {
                return helper->get(&node).findAllCallees();
              },
              visitFn);
      //std::cout << "Functions on call path from " << fn << ": " << count << "\n";
    } else if constexpr (Dir == TraverseDir::TraverseUp) {
      int count = traverseCallGraph(
              *fn, [this](const metacg::CgNode & node) -> auto {
                return helper->get(&node).findAllCallers();
              },
              visitFn);
      //std::cout << "Functions on call path to " << fn << ": " << count << "\n";
    }
  }
  return out;
}


}


#endif //CAPI_CALLPATHSELECTOR_H
