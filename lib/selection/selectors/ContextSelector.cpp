//
// Created by sebastian on 29.06.23.
//

#include "ContextSelector.h"

namespace capi {

static std::vector<CGNode*> bfsStep(const std::vector<CGNode*>& workingSet, const std::vector<CGNode*>& excludeList, const CallGraph& cg) {
  std::vector<CGNode*> addedSet;
  for (auto& node : workingSet) {
    if (setContains(excludeList, node))
      continue;
    for (auto* parent : node->getCallers()) {
      if (!setContains(workingSet, parent)) {
        addToSet(addedSet, parent);
      }
    }
  }
  return addedSet;
}


FunctionSet ContextSelector::apply(const FunctionSetList& input) {
  if (input.size() != 2) {
    logError() << "Expected exactly two input sets, got " << input.size() << " instead.\n";
    return {};
  }

  auto& inputA = input[0];
  auto& inputB = input[1];

  auto checkInput = [&](const FunctionSet& inSet) -> bool {
    if (inSet.empty())
      return false;
    if (inSet.size() > 1) {
      logInfo() << "Warning: Input for ContextSelector contains more than one element (" << inSet.size() << ")\n";
      logInfo() << "Continuing with the first element and ignoring the rest: " << inSet.front() << "\n";
    }
    return true;
  };
  if (!(checkInput(inputA) && checkInput(inputB))) {
    logError() << "Input set for ContextSelector is empty.\n";
    return {};
  }

  auto& targetNodeA = inputA.front();
  auto& targetNodeB = inputB.front();

  std::vector<CGNode*> setA{cg->get(targetNodeA)};
  std::vector<CGNode*> setB{cg->get(targetNodeB)};

  std::vector<CGNode*> commonAncestors{};

  std::vector<CGNode*> completedNodes{};

  FunctionSet selection;

  constexpr int maxExpansionSteps = 50;
  int steps = 0;

  // Find the first common ancestor by expanding in a breadth-first fashion
  for (; steps < maxExpansionSteps; steps++) {
    // Expand BFS
    auto addedA = bfsStep(setA, completedNodes, *cg);
    auto addedB = bfsStep(setB, completedNodes, *cg);

//    logInfo() << "Set A:\n";
//    for (auto& a : setA) {
//      logInfo() << a->getName() << "\n";
//    }
//    logInfo() << "Set B:\n";
//    for (auto& b : setB) {
//      logInfo() << b->getName() << "\n";
//    }
//
//    logInfo() << "Added " << (addedB.size() + addedA.size()) << " functions\n";
//    for (auto& a : addedA) {
//      logInfo() << a->getName() << "\n";
//    }
//    for (auto& b : addedB) {
//      logInfo() << b->getName() << "\n";
//    }
//    logInfo() << "--------------------------\n";

    // Abort condition - No new ancestors found
    if (addedA.empty() || addedB.empty()) {
      break;
    }

    // Update working sets
    setA.insert(setA.end(), addedA.begin(), addedA.end());
    setB.insert(setB.end(), addedB.begin(), addedB.end());

    // Check for intersection
    auto intersectA = intersect(addedA, setB);
    auto intersectB = intersect(addedB, setA);

    if (!intersectA.empty() || !intersectB.empty()) {
      commonAncestors.insert(commonAncestors.end(), intersectA.begin(), intersectA.end());
      // Avoid duplicates
      for (auto& ib: intersectB) {
        addToSet(commonAncestors, ib);
      }

      logInfo() << "Found " << commonAncestors.size() << " common ancestor(s). First one is " << commonAncestors.front()->getName() << "\n";

      // Find paths from ancestor to target nodes:
      // Start at ancestor. Recursively select children, if they are part of one of the BFS sets.

      std::vector<CGNode*> pathNodes;
      for (auto& ca : commonAncestors) {
        std::vector<CGNode*> workingSet;
        workingSet.push_back(ca);
        while(!workingSet.empty()) {
          auto node = workingSet.back();
          workingSet.pop_back();
          addToSet(pathNodes, node);
          // Also add to overall selection result
          addToSet(selection, node->getName());
          for (auto callee : node->getCallees()) {
            if (setContains(setA, callee) || setContains(setB, callee)) {
              addToSet(workingSet, callee);
            }
          }
        }
      }
      completedNodes.insert(completedNodes.end(), pathNodes.begin(), pathNodes.end());
      commonAncestors.clear();
    }
  }

  logInfo() << "Context selection terminated after " << steps << " expansion steps (max: " << maxExpansionSteps << ")\n";

  if (selection.empty()) {
    logInfo() << "Could not find a common ancestor.\n";
  }

  return selection;
}

}
