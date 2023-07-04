//
// Created by sebastian on 29.06.23.
//

#include "ContextSelector.h"

#ifndef CAPI_TRAVERSE_VIRTUAL_DESTRUCTORS
#define CAPI_TRAVERSE_VIRTUAL_DESTRUCTORS false
#endif


namespace capi {

static std::vector<const CGNode*> bfsStep(const std::vector<const CGNode*>& workingSet, const std::vector<const CGNode*>& excludeList, const CallGraph& cg) {
  std::vector<const CGNode*> addedSet;
  for (auto& node : workingSet) {
    if (setContains(excludeList, node))
      continue;
    for (const auto* parent : node->getCallers()) {
      if (!setContains(workingSet, parent)) {
        addToSet(addedSet, parent);
      }
    }
    // In addition to the direct parent, add the parents of all functions that are overridden by the current function.
    for (auto& overrides : node->findAllOverrides()) {
      for (auto* overrideParent : overrides->getCallers()) {
        if (!setContains(workingSet, overrideParent)) {
          addToSet(addedSet, overrideParent);
        }
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
      logInfo() << "Continuing with the first element and ignoring the rest: " << (*inSet.begin())->getName() << "\n";
    }
    return true;
  };
  if (!(checkInput(inputA) && checkInput(inputB))) {
    logError() << "Input set for ContextSelector is empty.\n";
    return {};
  }

  auto& targetNodeA = *inputA.begin();
  auto& targetNodeB = *inputB.begin();

  std::vector<const CGNode*> setA{targetNodeA};
  std::vector<const CGNode*> setB{targetNodeB};

  std::vector<const CGNode*> commonAncestors{};

  std::vector<const CGNode*> completedNodes{};

  FunctionSet selection;

  constexpr int maxExpansionSteps = 10;
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

      logInfo() << "Found " << commonAncestors.size() << " common ancestor(s). First one is " << commonAncestors.begin()->getName() << "\n";

      // Find paths from ancestor to target nodes:
      // Start at ancestor. Recursively select children, if they are part of one of the BFS sets.

for (auto& ca : commonAncestors) {
        std::vector<const CGNode*> pathNodes;
        logInfo() << "  " << ca->getName() << ":\n";
        std::vector<const CGNode*> workingSet;
        workingSet.push_back(ca);
        while(!workingSet.empty()) {
          auto node = workingSet.back();
          workingSet.pop_back();
          addToSet(pathNodes, node);
          logInfo() << "    " << node->getName() << "\n";
          // Also add to overall selection result
          addToSet(selection, node);
          // Add all callees that are on the path
          for (const auto* callee : node->getCallees()) {
            // Avoid getting stuck in potential cycles
            if (setContains(pathNodes, callee)) {
              continue;
            }
            if (setContains(setA, callee) || setContains(setB, callee)) {
              addToSet(workingSet, callee);
            }
            // Also consider function that override this caller
            for (auto& overridesCallee : callee->findAllOverriddenBy()) {
              // FIXME: Currently, we exclude destructors, as they are not properly represented in MetaCG.
              if (!CAPI_TRAVERSE_VIRTUAL_DESTRUCTORS && overridesCallee->isDestructor()) {
                continue;
              }
              if (setContains(pathNodes, overridesCallee)) {
                continue;
              }
              if (setContains(setA, overridesCallee) || setContains(setB, overridesCallee)) {
                addToSet(workingSet, overridesCallee);
              }
            }
          }
        }
        completedNodes.insert(completedNodes.end(), pathNodes.begin(), pathNodes.end());
      }
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
