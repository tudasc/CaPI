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

/**
 * Traverses the call graph, calling the given visit function on each node.
 * @tparam TraverseFn Function that takes a metacg::CgNode& argument and returns the next
 * nodes to traverse.
 * @tparam VisitFn Function that takes a metacg::CgNode& argument.
 * @param node
 * @param visit
 * @returns The number of visited functions.
 */
template <typename TraverseFn, typename VisitFn>
int traverseCallGraph(const metacg::CgNode &node, TraverseFn &&selectNextNodes,
                      VisitFn &&visit) {
  std::vector<const metacg::CgNode *> workingSet;
  std::unordered_set<const metacg::CgNode *> alreadyVisited;

  workingSet.push_back(&node);

  do {
    auto currentNode = workingSet.back();
    workingSet.pop_back();
    //        std::cout << "Visiting caller " << currentNode->getName() << "\n";
    visit(*currentNode);
    alreadyVisited.insert(currentNode);
    for (auto &nextNode : selectNextNodes(*currentNode)) {
      if (!alreadyVisited.contains(nextNode)
          && std::find(alreadyVisited.begin(), alreadyVisited.end(), nextNode) == alreadyVisited.end()) {
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
