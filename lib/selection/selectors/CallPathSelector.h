//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_CALLPATHSELECTOR_H
#define CAPI_CALLPATHSELECTOR_H

#include "Selector.h"

namespace capi {

enum class TraverseDir { TraverseUp, TraverseDown };

template <TraverseDir dir> class CallPathSelector : public Selector {
  CallGraph *cg;

public:
  CallPathSelector() = default;

  void init(CallGraph &cg) override {
    this->cg = &cg;
  }

  FunctionSet apply(const FunctionSetList& input) override;

  std::string getName() override {
    if constexpr (dir == TraverseDir::TraverseUp) {
      return "CallPathSelector<TraverseUp>";
    }
    return "CallPathSelector<TraverseDown>";
  }
};

/**
 * Traverses the call chain downwards, calling the given visit function on each
 * node.
 * @tparam VisitFn Function that takes a CGNode& argument and returns the next
 * nodes to traverse.
 * @tparam VisitFn Function that takes a CGNode& argument.
 * @param node
 * @param visit
 * @returns The number of visited functions.
 */
template <typename TraverseFn, typename VisitFn>
int traverseCallGraph(CGNode &node, TraverseFn &&selectNextNodes,
                      VisitFn &&visit) {
  std::vector<CGNode *> workingSet;
  std::vector<CGNode *> alreadyVisited;

  workingSet.push_back(&node);

  do {
    auto currentNode = workingSet.back();
    workingSet.pop_back();
    //        std::cout << "Visiting caller " << currentNode->getName() << "\n";
    visit(*currentNode);
    alreadyVisited.push_back(currentNode);
    for (auto &nextNode : selectNextNodes(*currentNode)) {
      if (std::find(workingSet.begin(), workingSet.end(), nextNode) ==
          workingSet.end() &&
          std::find(alreadyVisited.begin(), alreadyVisited.end(), nextNode) ==
          alreadyVisited.end()) {
        workingSet.push_back(nextNode);
      }
    }

  } while (!workingSet.empty());

  return alreadyVisited.size();
}

template <TraverseDir Dir> FunctionSet CallPathSelector<Dir>::apply(const FunctionSetList& input) {
  static_assert(Dir == TraverseDir::TraverseDown ||
                Dir == TraverseDir::TraverseUp);

  if (input.size() != 1) {
    logError() << "Expected exactly one input sets, got " << input.size() << "instead.\n";
    return {};
  }


  FunctionSet in = input.front();
  FunctionSet out(in);

  auto visitFn = [&out](CGNode &node) {
    if (std::find(out.begin(), out.end(), node.getName()) == out.end()) {
      out.push_back(node.getName());
    }
  };

  for (auto &fn : in) {
    auto fnNode = cg->get(fn);

    if constexpr (Dir == TraverseDir::TraverseDown) {
      int count = traverseCallGraph(
              *fnNode, [](CGNode & node) -> auto { return node.getCallees(); },
              visitFn);
      std::cout << "Functions on call path from " << fn << ": " << count << "\n";
    } else if constexpr (Dir == TraverseDir::TraverseUp) {
      int count = traverseCallGraph(
              *fnNode, [](CGNode & node) -> auto { return node.getCallers(); },
              visitFn);
      std::cout << "Functions on call path to " << fn << ": " << count << "\n";
    }
  }
  return out;
}


}


#endif //CAPI_CALLPATHSELECTOR_H
