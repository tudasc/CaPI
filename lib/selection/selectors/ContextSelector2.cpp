//
// Created by sebastian on 29.06.23.
//

#include "ContextSelector2.h"

#include <deque>
#include <unordered_map>
#include <unordered_set>

#include "SCC.h"

namespace capi {

struct CommonCallerSearchNodeData {
  const CGNode* node{nullptr};
  int distA{-1};
  int distB{-1};
  int lcaDist{-1};
  bool isInteresting{false};
  bool isLCA{false};
  const CGNode* predA{nullptr};
  const CGNode* predB{nullptr};
  std::unordered_set<const CommonCallerSearchNodeData*> lcaDescendants{};

  bool canReachExactlyOne() const {
    return (distA >= 0) != (distB >= 0);
  }

  bool isCA() const {
    return predA && predB;
  }
};

std::ostream& operator<<(std::ostream& os, const CommonCallerSearchNodeData& nd) {
   return os << (nd.node ? nd.node->getName() : "null") << " (" << nd.distA << ", " << nd.distB << ", " << (nd.isLCA ? "true" : "false") << ")";
}

FunctionSet ContextSelector2::apply(const FunctionSetList& input) {
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

  const auto* targetNodeA = *inputA.begin();
  const auto* targetNodeB = *inputB.begin();

  assert(targetNodeA && targetNodeB);

  SCCAnalysisResults sccResults = computeSCCs(*cg, true);

  if (sccResults.getSCC(*targetNodeA) == sccResults.getSCC(*targetNodeB)) {
    logError() << "Target nodes are located in the same SCC\n";
    return {};
  }

  std::unordered_map<const CGNode*, CommonCallerSearchNodeData> nodeDataMap {{targetNodeA, {targetNodeA, 0, -1, -1, false, false, nullptr, nullptr, {}}},
                                                                       {targetNodeB, {targetNodeB, -1, 0, -1, false, false, nullptr, nullptr, {}}}};

  std::vector<CommonCallerSearchNodeData*> commonAncestors;

  std::deque<CommonCallerSearchNodeData*> workQueue{&nodeDataMap[targetNodeA], &nodeDataMap[targetNodeB]};

  auto addToQueue = [&workQueue](CommonCallerSearchNodeData* data) {
    if (std::find(workQueue.begin(), workQueue.end(), data) ==
        workQueue.end()) {
      workQueue.push_back(data);
    }
  };

  do {
    auto nodeData = workQueue.front();
    workQueue.pop_front();

    std::cout << "Working on " << *nodeData << "\n";

    for (auto& caller : nodeData->node->findAllCallers()) {

      // TODO: Use shrinking mechanism
      //auto callerSCCSize = sccResults.getSCCSize(*caller);
      //if (callerSCCSize > 1)  {
      //  std::cout << "Caller is in SCC of size " << callerSCCSize << ": " << caller->getName() << " - skipping...\n";
      //  continue;
      //}

      auto& callerData = nodeDataMap[caller];
      bool relaxCaller{false};

      if (!callerData.node) {
        callerData.node = caller;
      }
      std::cout << "Caller: " << callerData << "\n";

      if (callerData.node == nodeData->node) {
        continue;
      }

      int altDistA = nodeData->distA + 1;
      if (nodeData->distA >= 0 && (callerData.distA < 0 || altDistA < callerData.distA)) {
        callerData.distA = altDistA;
        callerData.predA = nodeData->node;
        relaxCaller = true;
      }
      int altDistB = nodeData->distB + 1;
      if (nodeData->distB >= 0 && (callerData.distB < 0 || altDistB < callerData.distB)) {
        callerData.distB = altDistB;
        callerData.predB = nodeData->node;
        relaxCaller = true;
      }
//      int altpLCAOrder = nodeData->pLCAOrder + 1;
//      if (nodeData->pLCAOrder >= 0 && (callerData.pLCAOrder < 0 || altpLCAOrder ))

      if (nodeData->isLCA && callerData.isLCA) {
        std::cout << callerData.node->getName() << " was marked as potential LCA, but is not anymore.\n";
        callerData.isLCA = false;
//        relaxCaller = true;
      }

//      int altLCADist = nodeData->lcaDist + 1;
//      if (nodeData->lcaDist >= 0 && (callerData.lcaDist < 0 || altLCADist > callerData.lcaDist)) {
//        callerData.lcaDist = altLCADist;
//        relaxCaller = true;
//      }

      if (relaxCaller) {
        std::cout << "Entering in queue after update " << callerData << "\n";
        if (callerData.isCA()) {
          std::cout << "Caller is common ancestor\n";
          addToSet(commonAncestors, &callerData);
          if (!nodeData->isLCA && !nodeData->isCA()) {
            callerData.isLCA = true;
            std::cout << "Found potential LCA: " << callerData.node->getName() << "\n";
          }
//          if (nodeData->lcaDist < 0) {
//            callerData.lcaDist = 0;
//          }
//          if (callerData.lcaDist == 0) {
//            std::cout << "Found potential LCA: " << callerData.node->getName() << "\n";
//          }
        }
        addToQueue(&callerData);
      }
    }

  } while(!workQueue.empty());

  assert(workQueue.empty());

  for (auto& ca : commonAncestors) {
    if (ca->isLCA) {
      // LCAs are by definition interesting
      ca->isInteresting = true;
      ca->lcaDist = 0;
      ca->lcaDescendants = {ca};
      addToQueue(ca);
    }
  }

  std::cout << "Number of LCAs: " << workQueue.size() << "\n";

  if (workQueue.empty()) {
    return {targetNodeA, targetNodeB};
  }

  // Determine "interesting" CAs
  // Definition of interesing v in CA(x,y):
  // - it has more than one LCA descendant or
  // - it has a child that is ancestor of either x or y, but not both

  std::vector<CommonCallerSearchNodeData*> interestingCAs;

  do {
    auto nodeData = workQueue.front();
    workQueue.pop_front();

    std::cout << "Working on " << *nodeData << "\n";

    // Check if current node is interesting
    if (!nodeData->isInteresting) {
      nodeData->isInteresting = nodeData->lcaDescendants.size() > 1;
      if (!nodeData->isInteresting) {
        for (auto &child : nodeData->node->findAllCallees()) {
          const auto &childData = nodeDataMap[child];
          // Check if child can reach one, but not the other target node
          if (childData.canReachExactlyOne()) {
            nodeData->isInteresting = true;
            break;
          }
        }
      }
    }
    if (nodeData->isInteresting) {
      std::cout << "Node is interesting!\n";
      addToSet(interestingCAs, nodeData);
    }

    int altLCADist = nodeData->lcaDist;
    if (nodeData->isInteresting) {
      altLCADist++;
    }

    for (auto& caller : nodeData->node->findAllCallers()) {

      auto& callerData = nodeDataMap[caller];
      auto oldSize = callerData.lcaDescendants.size();
      std::cout << "Caller: " << callerData << "\n";

      // Update interesting descendants of caller
//      if (nodeData->isInteresting) {
//        callerData.interestingDescendants.insert(nodeData);
//      }
      callerData.lcaDescendants.insert(nodeData->lcaDescendants.begin(), nodeData->lcaDescendants.end());

      // Update LCA distance
      bool updateLCADist = callerData.lcaDist < 0 || altLCADist < callerData.lcaDist;
      if (updateLCADist) {
        callerData.lcaDist = altLCADist;
        std::cout << "Updated LCA dist: " << callerData.lcaDist << "\n";
      }

      // Only add caller to queue if something changed
      if (updateLCADist || callerData.lcaDescendants.size() != oldSize) {
        addToQueue(&callerData);
      }

    }
  } while(!workQueue.empty());

  assert(workQueue.empty());

  std::cout << "Number of interesting CAs: " << interestingCAs.size() << "\n";

  // Select all interesting CAs with lcaDist <= maxLCADist
  for (auto& ca : interestingCAs) {
    if (ca->lcaDist <= maxLCADist) {
      // TODO: Output trigger information as part of selection result. Also add option for user to control if this should be set at all.
      ca->node->isTrigger = true;
      addToQueue(ca);
    }
  }

  std::cout << "Number of interesting CAs with LCADist <= " << maxLCADist << ": " << workQueue.size() << "\n";

  // Allways select the target functions
  FunctionSet out{targetNodeA, targetNodeB};

  if (workQueue.empty()) {
    return out;
  }

  do {
    auto nodeData = workQueue.front();
    workQueue.pop_front();
    const auto* node = nodeData->node;
    addToSet(out, node);
    nodeData->node = nullptr; // Using this as a marker for processed nodes to prevent cycles.
    for (auto& callee : node->findAllCallees()) {
      auto& calleeData = nodeDataMap[callee];
      if (!calleeData.node) {
        // This node has already been visited, skip.
        continue;
      }
      if (calleeData.predA || calleeData.predB) {
        addToQueue(&calleeData);
      }
    }
  } while (!workQueue.empty());

  return out;
}

}
