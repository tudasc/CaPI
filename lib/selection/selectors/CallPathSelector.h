//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_CALLPATHSELECTOR_H
#define CAPI_CALLPATHSELECTOR_H

#include "Selector.h"

namespace capi {

enum class TraverseDir { TraverseUp, TraverseDown };

template <TraverseDir dir> class CallPathSelector : public Selector {
  CallGraph *cg{nullptr};

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
  std::vector<const CGNode *> workingSet;
  std::vector<const CGNode *> alreadyVisited;

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
    logError() << "Expected exactly one input sets, got " << input.size() << " instead.\n";
    return {};
  }


  FunctionSet in = input.front();
  FunctionSet out(in);

  auto visitFn = [&out](const CGNode &node) {
    if (std::find(out.begin(), out.end(), node.getName()) == out.end()) {
      out.push_back(node.getName());
    }
  };

  for (auto &fn : in) {
    auto fnNode = cg->get(fn);

    if constexpr (Dir == TraverseDir::TraverseDown) {
      int count = traverseCallGraph(
              *fnNode, [](const CGNode & node) -> auto {
                // TODO: This could be expressed more elegantly (and probably efficiently) using ranges::concat, but this is not in the standard yet.
                // If the current implementation proves to be a bottleneck, we can try caching a combined list of callers/callees and overriding functions.
                std::vector<const CGNode*> ancestors(node.getCallees().begin(), node.getCallees().end());
                // Add functions that override any of the callees.
                std::for_each(node.getCallees().begin(), node.getCallees().end(), [&ancestors] (const auto* callee) {
                  auto allOverriddenBy = callee->findAllOverriddenBy();
                  ancestors.insert(ancestors.end(), allOverriddenBy.begin(), allOverriddenBy.end());
                });
                return ancestors;
              },
              visitFn);
      //std::cout << "Functions on call path from " << fn << ": " << count << "\n";
    } else if constexpr (Dir == TraverseDir::TraverseUp) {
      int count = traverseCallGraph(
              *fnNode, [](const CGNode & node) -> auto {
                // TODO: This could be expressed more elegantly (and probably efficiently) using ranges::concat, but this is not in the standard yet.
                // If the current implementation proves to be a bottleneck, we can try caching a combined list of callers/callees and overriding functions.
                std::vector<const CGNode*> predecessors(node.getCallers().begin(), node.getCallers().end());
                // Add callers of each function that overrides the current function
                auto allOverrides = node.findAllOverrides();
                std::for_each(allOverrides.begin(), allOverrides.end(), [&predecessors] (const auto* overrides) {
                  predecessors.insert(predecessors.end(), overrides->getCallers().begin(), overrides->getCallers().end());
                });

                return predecessors;
              },
              visitFn);
      //std::cout << "Functions on call path to " << fn << ": " << count << "\n";
    }
  }
  return out;
}


}


#endif //CAPI_CALLPATHSELECTOR_H
