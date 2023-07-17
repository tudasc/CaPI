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
  bool isLCA{false};
  const CGNode* predA{nullptr};
  const CGNode* predB{nullptr};
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

  std::unordered_map<const CGNode*, CommonCallerSearchNodeData> nodeDataMap {{targetNodeA, {targetNodeA, 0, -1, false, nullptr, nullptr}},
                                                                       {targetNodeB, {targetNodeB, -1, 0, false, nullptr, nullptr}}};

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
      if (nodeData->isLCA && callerData.isLCA) {
        std::cout << callerData.node->getName() << " was marked as potential LCA, but is not anymore.\n";
        callerData.isLCA = false;
        //relaxCaller = true;
      }

//      int altLCADist = nodeData->lcaDist + 1;
//      if (nodeData->lcaDist >= 0 && (callerData.lcaDist < 0 || altLCADist > callerData.lcaDist)) {
//        callerData.lcaDist = altLCADist;
//        relaxCaller = true;
//      }

      if (relaxCaller) {
        std::cout << "Entering in queue after update " << callerData << "\n";
        if (callerData.predA && callerData.predB) {
          std::cout << "Caller is common ancestor\n";
          addToSet(commonAncestors, &callerData);
          if (!nodeData->isLCA && !(nodeData->predA && nodeData->predB)) {
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
  // Select all nodes from LCAs that can reach either A or B
  for (auto& ca : commonAncestors) {
    if (ca->isLCA) {
      // TODO: Output this as part of selection result. Also add option for user to control if this should be set at all.
      ca->node->isTrigger = true;
      addToQueue(ca);
    }
  }
  std::cout << "Number of LCAs: " << workQueue.size() << "\n";

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
