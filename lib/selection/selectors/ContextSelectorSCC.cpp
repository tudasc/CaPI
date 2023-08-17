//
// Created by sebastian on 29.06.23.
//

#include "ContextSelectorSCC.h"

#include <deque>
#include <unordered_map>
#include <unordered_set>

#include "SCC.h"

namespace capi {

struct CommonCallerSearchNodeDataSCC {
  const SCCNode* node{nullptr};
  int distA{-1};
  int distB{-1};
  int lcaDist{-1};
  bool isCandidate{false};
  bool isLCA{false};
  bool isDistinct{false};
  bool isPartiallyDistinct{false};
  const SCCNode* predA{nullptr};
  const SCCNode* predB{nullptr};
  std::unordered_set<const CommonCallerSearchNodeDataSCC*> lcaDescendants{};

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

std::ostream& operator<<(std::ostream& os, const CommonCallerSearchNodeDataSCC& nd) {
   return os << (nd.node ? nd.node->nodes.front()->getName() : "null") << " (" << nd.distA << ", " << nd.distB << ", " << (nd.isLCA ? "true" : "false") << ")";
}

FunctionSet ContextSelectorSCC::apply(const FunctionSetList& input) {
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

  const auto* targetSCCA = sccResults.getSCC(*targetNodeA);
  const auto* targetSCCB = sccResults.getSCC(*targetNodeB);

  if (targetSCCA == targetSCCB) {
    logError() << "Target nodes are located in the same SCC\n";
    return {};
  }


  std::unordered_map<const SCCNode*, CommonCallerSearchNodeDataSCC> sccDataMap {{targetSCCA, {targetSCCA, 0, -1, -1, false, false, false, false, nullptr, nullptr, {}}},
                                                                             {targetSCCB, {targetSCCB, -1, 0, -1, false, false, false, false, nullptr, nullptr, {}}}};

  std::vector<CommonCallerSearchNodeDataSCC*> commonAncestors;

  std::deque<CommonCallerSearchNodeDataSCC*> workQueue{&sccDataMap[targetSCCA], &sccDataMap[targetSCCB]};

  auto addToQueue = [&workQueue](CommonCallerSearchNodeDataSCC* data) {
    if (std::find(workQueue.begin(), workQueue.end(), data) ==
        workQueue.end()) {
      workQueue.push_back(data);
    }
  };

  do {
    auto nodeData = workQueue.front();
    workQueue.pop_front();

    //std::cout << "Working on " << *nodeData << "\n";

    for (auto& caller : sccResults.findAllCallers(nodeData->node)) {

      auto& callerData = sccDataMap[caller];
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
      auto callerSCCSize = ca->node->size();
      if (callerSCCSize > 1) {
        std::cout << "LCA is SCC of size " << callerSCCSize << ", first node: "
                  << ca->node->nodes.front()->getName() << " - skipping...\n";
        continue;
      }
      // LCAs are by definition candidates
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

  // Determine candidate CAs
  // Definition of candidate v in CA(x,y):
  // - it has more than one LCA descendant or
  // - it has a child that is ancestor of either x or y, but not both

  std::vector<CommonCallerSearchNodeDataSCC*> candidates;

  do {
    auto nodeData = workQueue.front();
    workQueue.pop_front();

   // std::cout << "Working on " << *nodeData << "\n";

    // Check if current node is candidate
    if (!nodeData->isCandidate) {
      nodeData->isCandidate = nodeData->lcaDescendants.size() > 1;
      if (!nodeData->isCandidate) {
        for (auto &child : sccResults.findAllCallees(nodeData->node)) {
          const auto &childData = sccDataMap[child];
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
      addToSet(candidates, nodeData);
      bool childReachesOnlyA = false;
      bool childReachesOnlyB = false;
      for (auto &child : sccResults.findAllCallees(nodeData->node)) {
        const auto &childData = sccDataMap[child];
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

    for (auto& caller : sccResults.findAllCallers(nodeData->node)) {

      auto& callerData = sccDataMap[caller];
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

  std::cout << "Number of candidate dLCAs: " << candidates.size() << "\n";

  constexpr int NumLCABuckets = 11;
  std::array<int, NumLCABuckets> lcaDistCount;
  std::array<int, NumLCABuckets> lcaDistCountDistinct;
  std::array<int, NumLCABuckets> lcaDistCountPartiallyDistinct;
  lcaDistCount.fill(0);
  lcaDistCountDistinct.fill(0);
  lcaDistCountPartiallyDistinct.fill(0);

  int numDistinct = 0;

  // Select all candidate CAs with lcaDist <= maxLCADist matching the heuristic
  for (auto& ca : candidates) {
    int cappedLcaDist = std::min(ca->lcaDist, NumLCABuckets-1);
    lcaDistCount[cappedLcaDist]++;
    bool consideredInHeuristic = type == CAHeuristicType::ALL;
    if (ca->isPartiallyDistinct) {
      lcaDistCountPartiallyDistinct[cappedLcaDist]++;
      if (!consideredInHeuristic && type != CAHeuristicType::DISTINCT) {
        consideredInHeuristic = true;
      }
    }
    if (ca->isDistinct) {
      lcaDistCountDistinct[cappedLcaDist]++;
//      if (ca->lcaDist > 1) {
//        logError() << "Candidate with LCA-Dist " << ca->lcaDist << " is marked as distinct!\n";
//        logError() << "SCC size: " << ca->node->size() << "\n";
//      }
      numDistinct++;
      consideredInHeuristic = true;
    }
    if (consideredInHeuristic && ca->lcaDist <= maxLCADist) {
      // TODO: Output trigger information as part of selection result. Also add option for user to control if this should be set at all.

      for (auto& member : ca->node->nodes) {
        member->isTrigger = true;
      }
      addToQueue(ca);
    }
  }

  auto getInstrumented = [&targetNodeA, &targetNodeB, &sccResults, &sccDataMap](std::deque<CommonCallerSearchNodeDataSCC*>& queue) {
    FunctionSet out{targetNodeA, targetNodeB};

    if (queue.empty()) {
      return out;
    }

    std::unordered_set<const SCCNode*> visited;

    do {
      auto nodeData = queue.front();
      queue.pop_front();
      const auto* node = nodeData->node;
      for (auto& member : node->nodes) {
        addToSet(out, member);
      }
      visited.insert(node);
      for (auto& callee : sccResults.findAllCallees(node)) {
        auto& calleeData = sccDataMap[callee];
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

      std::deque<CommonCallerSearchNodeDataSCC*> candidateQ;
      std::deque<CommonCallerSearchNodeDataSCC*> partiallyDistinctQ;
      std::deque<CommonCallerSearchNodeDataSCC*> distinctQ;

      for (auto &ca : candidates) {
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

    for (int i = 0; i < NumLCABuckets - 1; i++) {
      std::cout << "Candidates with LCA-Dist = " << i << ": " << lcaDistCount[i]
                << std::endl;

      std::cout << "  of these partially distinct: "
                << lcaDistCountPartiallyDistinct[i] << std::endl;
      std::cout << "  of these distinct: "
                << lcaDistCountDistinct[i] << std::endl;
    }
    std::cout << "Candidates with LCA-Dist >= " << NumLCABuckets - 1 << ": "
              << lcaDistCount.back() << std::endl;
    std::cout << "  of these partially distinct: "
              << lcaDistCountPartiallyDistinct.back() << std::endl;
    std::cout << "  of these distinct: "
              << lcaDistCountDistinct.back() << std::endl;

    // Number of instrumented function for each bucket

  } else {
    std::cout << "Number of candidate dLCAs with LCADist <= " << maxLCADist << ": " << workQueue.size() << "\n";
  }

  FunctionSet out = getInstrumented(workQueue);

//  do {
//    auto nodeData = workQueue.front();
//    workQueue.pop_front();
//    const auto* node = nodeData->node;
//    addToSet(out, node);
//    nodeData->node = nullptr; // Using this as a marker for processed nodes to prevent cycles.
//    for (auto& callee : node->findAllCallees()) {
//      auto& calleeData = nodeDataMap[callee];
//      if (!calleeData.node) {
//        // This node has already been visited, skip.
//        continue;
//      }
//      if (calleeData.predA || calleeData.predB) {
//        addToQueue(&calleeData);
//      }
//    }
//  } while (!workQueue.empty());

  return out;
}

}
