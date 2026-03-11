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
  int maxDepth{0};

public:
  CallPathSelector() = default;
  CallPathSelector(int maxDepth) : maxDepth(maxDepth) {};

  void init(TraversalHelper &helper) override {
    this->helper = &helper;
  }

  FunctionSet apply(const FunctionSetList& input) override;

  std::string getName() override {
    if constexpr (dir == TraverseDir::TraverseUp) {
      return "CallPathSelector<TraverseUp>(" + std::to_string(maxDepth) + ")";
    }
    return "CallPathSelector<TraverseDown>("+ std::to_string(maxDepth) + ")";
  }
};

/**
 * Traverses the call chain downwards, calling the given visit function on each
 * node.
 * @tparam VisitFn Function that takes a metacg::CgNode& argument and the current call depth and returns the next
 * nodes to traverse.
 * @tparam VisitFn Function that takes a metacg::CgNode& argument.
 * @param node
 * @param visit
 * @returns The number of visited functions.
 */
template <typename TraverseFn, typename VisitFn>
int traverseCallGraph(const metacg::CgNode &node, TraverseFn &&selectNextNodes,
                      VisitFn &&visit) {
//  struct NodeInfo {
//      const metacg::CgNode* node{nullptr};
//      int depth{0};
//  };



  std::vector<const CgNode*> workingSet;
  std::unordered_set<const metacg::CgNode*> alreadyVisited;
  std::unordered_map<const metacg::CgNode*, int> minDepth;

  workingSet.push_back(&node);
  minDepth[&node] = 0;

  do {
    auto currentNode = workingSet.back();
    workingSet.pop_back();
    //        std::cout << "Visiting caller " << currentNode->getName() << "\n";
    visit(*currentNode);
    int depth = minDepth[currentNode];

    alreadyVisited.insert(currentNode);
    for (auto &nextNode : selectNextNodes(*currentNode, depth)) {

        if (!minDepth.contains(nextNode) || depth+1 < minDepth[nextNode]) {
            minDepth[nextNode] = depth+1;
        }

      // Already visited? Check if we're at a lower depth
      if (auto vNode = std::find(alreadyVisited.begin(), alreadyVisited.end(), nextNode); vNode !=
          alreadyVisited.end()) {
          if (depth+1 >= minDepth[*vNode]) {
              continue;
          }
      }

      if (std::find(workingSet.begin(), workingSet.end(),nextNode) == workingSet.end()) {
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
              *fn, [this](const metacg::CgNode & node, int depth) -> auto {
                  auto callees = helper->get(&node).findAllCallees();
                if (maxDepth == 0 || depth < maxDepth)
                    return callees;
                return IterRange{callees.begin(), callees.begin()};
              },
              visitFn);
      //std::cout << "Functions on call path from " << fn << ": " << count << "\n";
    } else if constexpr (Dir == TraverseDir::TraverseUp) {
      int count = traverseCallGraph(
              *fn, [this](const metacg::CgNode & node, int depth) -> auto {
                  auto callers = helper->get(&node).findAllCallers();
                  if (maxDepth == 0 || depth < maxDepth)
                    return callers;
                  return IterRange{callers.begin(), callers.begin()};
              },
              visitFn);
      //std::cout << "Functions on call path to " << fn << ": " << count << "\n";
    }
  }
  return out;
}


}


#endif //CAPI_CALLPATHSELECTOR_H
