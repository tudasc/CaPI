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
  bool isCandidate{false};
  bool isLCA{false};
  bool isDistinct{false};
  bool isPartiallyDistinct{false};
  const CGNode* predA{nullptr};
  const CGNode* predB{nullptr};
  std::unordered_set<const CommonCallerSearchNodeData*> lcaDescendants{};

  bool reachesA() const {
      return distA >= 0;
  };

  bool reachesB() const {
      return distB >= 0;
  };

  bool canReachExactlyOne() const {
    return reachesA() != reachesB();
  }

  bool isCA() const {
    return reachesA() && reachesB();
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

  std::unordered_map<const CGNode*, CommonCallerSearchNodeData> nodeDataMap {{targetNodeA, {targetNodeA, 0, -1, -1, false, false, false, false, nullptr, nullptr, {}}},
                                                                       {targetNodeB, {targetNodeB, -1, 0, -1, false, false, false, false, nullptr, nullptr, {}}}};

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

    //std::cout << "Working on " << *nodeData << "\n";

    for (auto& caller : nodeData->node->findAllCallers()) {

      auto& callerData = nodeDataMap[caller];
      bool relaxCaller{false};

      if (!callerData.node) {
        callerData.node = caller;
      }
      //std::cout << "Caller: " << callerData << "\n";

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
//        std::cout << callerData.node->getName() << " was marked as potential LCA, but is not anymore.\n";
        callerData.isLCA = false;
//        relaxCaller = true;
      }

//      int altLCADist = nodeData->lcaDist + 1;
//      if (nodeData->lcaDist >= 0 && (callerData.lcaDist < 0 || altLCADist > callerData.lcaDist)) {
//        callerData.lcaDist = altLCADist;
//        relaxCaller = true;
//      }

      if (relaxCaller) {
        //std::cout << "Entering in queue after update " << callerData << "\n";
        if (callerData.isCA()) {
          //std::cout << "Caller is common ancestor\n";
          addToSet(commonAncestors, &callerData);
          if (!nodeData->isLCA && !nodeData->isCA()) {
            callerData.isLCA = true;
//            std::cout << "Found potential LCA: " << callerData.node->getName() << "\n";
          }

//          auto scc = sccResults.getSCC(*caller);
//          auto callerSCCSize = scc->size();
//          if (callerSCCSize > 1) {
//            std::cout << "Caller is CA and in SCC of size " << callerSCCSize << ": "
//                      << caller->getName() << " - skipping...\n";
////            FunctionSet out;
////            for (auto &node : *scc) {
////              addToSet(out, node);
////            }
////            return out;
//          }

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
      // Ignore CAs in cycles
      auto scc = sccResults.getSCC(*ca->node);
      auto callerSCCSize = scc->size();
      if (callerSCCSize > 1) {
        std::cout << "LCA is in SCC of size " << callerSCCSize << ": "
                  << ca->node->getName() << " - skipping...\n";
        continue;
      }
      // LCAs are by definition interesting
      ca->isCandidate = true;
      ca->isDistinct = true;
      ca->isPartiallyDistinct = true;
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

  std::vector<CommonCallerSearchNodeData*> candidateLCAs;

  do {
    auto nodeData = workQueue.front();
    workQueue.pop_front();

   // std::cout << "Working on " << *nodeData << "\n";

    // Check if current node is interesting
    if (!nodeData->isCandidate) {
      bool strongCandidate = false;
      nodeData->isCandidate = nodeData->lcaDescendants.size() > 1;
      if (!nodeData->isCandidate) {
        for (auto &child : nodeData->node->findAllCallees()) {
          const auto &childData = nodeDataMap[child];
          // Check if child can reach one, but not the other target node
          if (childData.canReachExactlyOne()) {
            nodeData->isCandidate = true;
            break;
          }
        }
      }
    }
    if (nodeData->isCandidate) {
      //std::cout << "Node is interesting!\n";
      addToSet(candidateLCAs, nodeData);
      bool childReachesOnlyA = false;
      bool childReachesOnlyB = false;
      for (auto &child : nodeData->node->findAllCallees()) {
        const auto &childData = nodeDataMap[child];
        if (childData.isCA())
          continue;
        if (childData.reachesA())
          childReachesOnlyA = true;
        else if (childData.reachesB())
          childReachesOnlyB = true;
      }
      if (childReachesOnlyA || childReachesOnlyB) {
        nodeData->isPartiallyDistinct = true;
        if (childReachesOnlyA && childReachesOnlyB) {
          nodeData->isDistinct = true;
        }
      }
    }

    int altLCADist = nodeData->lcaDist;
    if (nodeData->isCandidate) {

      altLCADist++;
    }

    for (auto& caller : nodeData->node->findAllCallers()) {

      auto& callerData = nodeDataMap[caller];
      auto oldSize = callerData.lcaDescendants.size();
      //std::cout << "Caller: " << callerData << "\n";

      // Update interesting descendants of caller
//      if (nodeData->isInteresting) {
//        callerData.interestingDescendants.insert(nodeData);
//      }
      callerData.lcaDescendants.insert(nodeData->lcaDescendants.begin(), nodeData->lcaDescendants.end());

      // Update LCA distance
      bool updateLCADist = callerData.lcaDist < 0 || altLCADist < callerData.lcaDist;
      if (updateLCADist) {
        callerData.lcaDist = altLCADist;
       // std::cout << "Updated LCA dist: " << callerData.lcaDist << "\n";
      }

      // Only add caller to queue if something changed
      if (updateLCADist || callerData.lcaDescendants.size() != oldSize) {
        addToQueue(&callerData);
      }

    }
  } while(!workQueue.empty());

  assert(workQueue.empty());

  std::cout << "Number of candidate dLCAs: " << candidateLCAs.size() << "\n";

  constexpr int NumLCABuckets = 11;
  std::array<int, NumLCABuckets> lcaDistCount;
  std::array<int, NumLCABuckets> lcaDistCountPartiallyDistinct;
  lcaDistCount.fill(0);
  lcaDistCountPartiallyDistinct.fill(0);

  int numDistinct = 0;

  // Select all interesting CAs with lcaDist <= maxLCADist
  for (auto& ca : candidateLCAs) {
    int cappedLcaDist = std::min(ca->lcaDist, NumLCABuckets-1);
    lcaDistCount[cappedLcaDist]++;
    if (ca->isPartiallyDistinct) {
      lcaDistCountPartiallyDistinct[cappedLcaDist]++;
    }
    if (ca->isDistinct) {
      if (ca->lcaDist > 1) {
        logError() << "Candidate with LCA-Dist " << ca->lcaDist << " is marked as distinct!\n";
      }
      numDistinct++;
    }
    if (ca->lcaDist <= maxLCADist) {
      // TODO: Output trigger information as part of selection result. Also add option for user to control if this should be set at all.
      ca->node->isTrigger = true;
      addToQueue(ca);
    }
  }

  auto getInstrumented = [&targetNodeA, &targetNodeB, &nodeDataMap](std::deque<CommonCallerSearchNodeData*>& queue) {
    FunctionSet out{targetNodeA, targetNodeB};

    if (queue.empty()) {
      return out;
    }

    std::unordered_set<const CGNode*> visited;

    do {
      auto nodeData = queue.front();
      queue.pop_front();
      const auto* node = nodeData->node;
      addToSet(out, node);
      visited.insert(node);
      for (auto& callee : node->findAllCallees()) {
        auto& calleeData = nodeDataMap[callee];
        if (setContains(visited, calleeData.node)) {
          // This node has already been visited, skip.
          continue;
        }
        if (calleeData.predA || calleeData.predB) {
          if (std::find(queue.begin(), queue.end(), &calleeData) ==
              queue.end()) {
            queue.push_back(&calleeData);
          };
        }
      }
    } while (!queue.empty());
    return out;
  };

  bool printCandidateStats = true;

  // Print stats
  if (printCandidateStats) {

    for (int i = 0; i < NumLCABuckets; i++) {

      std::deque<CommonCallerSearchNodeData*> candidateQ;
      std::deque<CommonCallerSearchNodeData*> partiallyDistinctQ;
      std::deque<CommonCallerSearchNodeData*> distinctQ;

      for (auto &ca : candidateLCAs) {
        int cappedLcaDist = std::min(ca->lcaDist, NumLCABuckets - 1);
        if (cappedLcaDist <= i) {
          candidateQ.push_back(ca);
          if (ca->isPartiallyDistinct) {
            partiallyDistinctQ.push_back(ca);
            if (ca->isDistinct) {
              distinctQ.push_back(ca);
            }
          }
        }
      }
      std::cout << "Candidates with LCA-Dist <= " << i << ": " << candidateQ.size()
                << std::endl;
      auto instrCandidates = getInstrumented(candidateQ);
      std::cout << " -> instrumented functions: " << instrCandidates.size() << std::endl;

      std::cout << "  of these partially distinct: "
                << partiallyDistinctQ.size() << std::endl;
      auto instrPartiallyDistinctCandidates = getInstrumented(partiallyDistinctQ);
      std::cout << "  -> instrumented functions: " << instrPartiallyDistinctCandidates.size() << std::endl;

      std::cout << "  of these fully distinct: "
                << distinctQ.size() << std::endl;
      auto instrDistinctCandidates = getInstrumented(distinctQ);
      std::cout << "   -> instrumented functions: " << instrDistinctCandidates.size() << std::endl;
    }

    std::cout << "------------------------------" << std::endl;

    std::cout << "Fully distinct candidates: " << numDistinct << std::endl;
    std::cout << "Fully distinct candidates (non-LCA): "
              << (numDistinct - lcaDistCount[0]) << std::endl;
    for (int i = 0; i < NumLCABuckets - 1; i++) {
      std::cout << "Candidates with LCA-Dist = " << i << ": " << lcaDistCount[i]
                << std::endl;

      std::cout << "  of these partially distinct: "
                << lcaDistCountPartiallyDistinct[i] << std::endl;
    }
    std::cout << "Candidates with LCA-Dist >= " << NumLCABuckets - 1 << ": "
              << lcaDistCount.back() << std::endl;
    std::cout << "  of these partially distinct: "
              << lcaDistCountPartiallyDistinct.back() << std::endl;

    // Number of instrumented function for each bucket

  } else {
    std::cout << "Number of candidate dLCAs with LCADist <= " << maxLCADist << ": " << workQueue.size() << "\n";
  }


  // Always select the target functions
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
