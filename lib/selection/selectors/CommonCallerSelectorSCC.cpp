//
// Created by sebastian on 29.06.23.
//

#include "CommonCallerSelectorSCC.h"

#include <deque>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

#include "DOTWriter.h"
#include "PostDom.h"
#include "SCC.h"

namespace capi {

struct CommonCallerSearchNodeDataSCC {
  const SCCNode* node{nullptr};
  int distA{-1};
  int distB{-1};
  int lcaDist{-1};
  int candidateDistA{-1};
  int candidateDistB{-1};
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

  int candidateDist() const {
    return std::max(candidateDistA, candidateDistB);
  }

  std::string getName() const {
    return node ? node->getName() : "NULL";
  }

};


std::ostream& operator<<(std::ostream& os, const CommonCallerSearchNodeDataSCC& nd) {
   return os << (nd.node ? nd.node->nodes.front()->getName() : "null") << " (" << nd.distA << ", " << nd.distB << ", " << (nd.isLCA ? "true" : "false") << ")";
}

FunctionSet CommonCallerSelectorSCC::apply(const FunctionSetList& input) {
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
      LOG_CRITICAL("Warning: Input for ContextSelector contains more than one element (" << inSet.size() << ")\n");
      LOG_CRITICAL("Continuing with the first element and ignoring the rest: " << (*inSet.begin())->getName() << "\n");
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


  std::unordered_map<const SCCNode*, CommonCallerSearchNodeDataSCC> sccDataMap {{targetSCCA, {targetSCCA, 0, -1, -1, 0, -1, false, false, false, false, nullptr, nullptr, {}}},
                                                                             {targetSCCB, {targetSCCB, -1, 0, -1, -1, 0, false, false, false, false, nullptr, nullptr, {}}}};

  std::vector<CommonCallerSearchNodeDataSCC*> commonAncestors;

  std::deque<CommonCallerSearchNodeDataSCC*> workQueue{&sccDataMap[targetSCCA], &sccDataMap[targetSCCB]};

  auto addToQueue = [&workQueue](CommonCallerSearchNodeDataSCC* data) {
    if (std::find(workQueue.begin(), workQueue.end(), data) ==
        workQueue.end()) {
      workQueue.push_back(data);
    }
  };

  auto makeReachableSubgraph = [&](CallGraph& subgraph, CommonCallerSearchNodeDataSCC* nodeData, DecorationMap& decoMap) {

    std::deque<CommonCallerSearchNodeDataSCC*> toProcess;
    toProcess.push_back(nodeData);

    do {
      auto current = toProcess.front();
      toProcess.pop_front();

      auto& subgraphNode = subgraph.getOrCreate(current->getName());

      for (auto& child : sccResults.findAllCallees(current->node)) {
        auto& childData = sccDataMap[child];
        if (childData.reachesA() || childData.reachesB()) {
          auto& childSubgraphNode = subgraph.getOrCreate(childData.getName());
          subgraph.addCallee(subgraphNode, childSubgraphNode, false);
          // Skip CAs to reduce graph
          if (childData.isCA()) {
            decoMap[childData.getName()].shapeColor = NodeDecoration::GREEN;
          }  else {
            decoMap[childData.getName()].shapeColor = childData.reachesA() ? NodeDecoration::RED : NodeDecoration::BLUE;
            if (std::find(toProcess.begin(), toProcess.end(), &childData) == toProcess.end()) {
              toProcess.push_back(&childData);
            }
          }

        }
      }

    } while (!toProcess.empty());
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

//      if (nodeData->isLCA && callerData.isLCA) {
////        std::cout << callerData.node->getName() << " was marked as potential LCA, but is not anymore.\n";
//        callerData.isLCA = false;
//        relaxCaller = true;
//      }

      if (relaxCaller) {
        //std::cout << "Entering in queue after update " << callerData << "\n";
        if (callerData.isCA()) {
          //std::cout << "Caller is common ancestor\n";
          addToSet(commonAncestors, &callerData);
//          if (!nodeData->isLCA && !nodeData->isCA()) {
//            callerData.isLCA = true;
////            std::cout << "Found potential LCA: " << callerData.node->getName() << "\n";
//          }
        }
        addToQueue(&callerData);
      }
    }

  } while(!workQueue.empty());

  assert(workQueue.empty());

  // Detect LCAs - CAs with out-degree zero
  for (auto& ca : commonAncestors) {
    bool isLCA = true;
    for (auto& callee : sccResults.findAllCallees(ca->node)) {
      if (auto entry = sccDataMap.find(callee); entry != sccDataMap.end() && entry->second.isCA()) {
        isLCA = false;
        break;
      }
    }
    ca->isLCA = isLCA;
  }

  // Compute postdominators w.r.t. A and B
  auto postDomsA = computePostDoms<SCCNode, SCCGraph> (SCCGraph(sccResults), *targetSCCA);
  auto postDomsB = computePostDoms<SCCNode, SCCGraph> (SCCGraph(sccResults), *targetSCCB);


  int numLCAs = 0;

  for (auto& ca : commonAncestors) {
    //LOG_STATUS("PostDoms(A) of CA " << dumpNodeSet(ca->node->nodes.front()->getName(), postDomsA[ca->node].postDoms) << "\n");
    //LOG_STATUS("PostDoms(B) of CA " << dumpNodeSet(ca->node->nodes.front()->getName(), postDomsB[ca->node].postDoms) << "\n");

    if (ca->isLCA) {

      // LCAs are by definition candidates
      ca->isCandidate = true;
      ca->isDistinct = true;
      ca->isPartiallyDistinct = true;
      ca->lcaDist = 0;

      numLCAs++;

//      ca->candidateDistA = 0;
//      ca->candidateDistB = 0;
//      ca->lcaDescendants = {ca};
//      addToQueue(ca);
    }
  }

  LOG_STATUS("Number of LCAs: " << numLCAs << "\n");

  if (numLCAs == 0) {
    return {targetNodeA, targetNodeB};
  }

  addToQueue(&sccDataMap[targetSCCA]);
  addToQueue(&sccDataMap[targetSCCB]);

  std::vector<CommonCallerSearchNodeDataSCC*> candidates;

  do {
    auto nodeData = workQueue.front();
    workQueue.pop_front();

//    logInfo() << "Processing node " << nodeData->getName() << ":\n";

    // Check if current node is candidate (LCA that have been filtered out are ignored)
    if (!nodeData->isCandidate && !nodeData->isLCA) {
      auto& nodePostDomsA = postDomsA[nodeData->node];
      auto& nodePostDomsB = postDomsB[nodeData->node];
      std::unordered_set<const SCCNode*> postDomIntersection;
      std::set_intersection(nodePostDomsA.postDoms.begin(), nodePostDomsA.postDoms.end(), nodePostDomsB.postDoms.begin(), nodePostDomsB.postDoms.end(), std::inserter(postDomIntersection, std::end(postDomIntersection)));
      // Candidate if node has no common postdominator w.r.t. A and B
      if (postDomIntersection.size() == 1 && (*postDomIntersection.begin()) == nodeData->node) {
        nodeData->isCandidate = true;
      }
    }

    if (nodeData->isCandidate) {
      // Ignore CAs in cycles
      auto callerSCCSize = nodeData->node->size();
      if (callerSCCSize == 1) {
        addToSet(candidates, nodeData);
      } else {
        LOG_CRITICAL("Candidate is SCC of size " << callerSCCSize << ", first node: "
                                           << nodeData->getName() << "\n");
        // TODO Testing including bigger SCCs
        addToSet(candidates, nodeData);

      }
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

    int altCDistA = nodeData->candidateDistA;
    int altCDistB = nodeData->candidateDistB;
//    int altLCADist = nodeData->lcaDist;

    // Increase candidate distance if candidate and reachable
    if (nodeData->isCandidate) {
      if (altCDistA >= 0) {
        altCDistA++;
      }
      if (altCDistB >= 0) {
        altCDistB++;
      }
    }

    for (auto& caller : sccResults.findAllCallers(nodeData->node)) {

      auto& callerData = sccDataMap[caller];


      bool updateCDistA = altCDistA >= 0 && (callerData.candidateDistA < 0 || altCDistA < callerData.candidateDistA);
      if (updateCDistA) {
        callerData.candidateDistA = altCDistA;
//        std::cout << callerData.node->getName() << ": updated A dist: " << callerData.candidateDistA << "\n";
      }

      bool updateCDistB = altCDistB >= 0 && (callerData.candidateDistB < 0 || altCDistB < callerData.candidateDistB);
      if (updateCDistB) {
        callerData.candidateDistB = altCDistB;
//        std::cout << callerData.node->getName() << ": updated B dist: " << callerData.candidateDistB << "\n";
      }

      // Only add caller to queue if something changed
      if (updateCDistA || updateCDistB) {
        addToQueue(&callerData);
      }

    }
  } while(!workQueue.empty());

  assert(workQueue.empty());

  LOG_STATUS("Number of candidate dLCAs: " << candidates.size() << "\n");

  constexpr int NumLCABuckets = 11;
  std::array<int, NumLCABuckets> candidateDistCount;
  std::array<int, NumLCABuckets> candidateDistCountDistinct;
  std::array<int, NumLCABuckets> candidateDistCountPartiallyDistinct;
  candidateDistCount.fill(0);
  candidateDistCountDistinct.fill(0);
  candidateDistCountPartiallyDistinct.fill(0);

  int numDistinct = 0;

  // Select all candidate CAs with lcaDist <= maxLCADist matching the heuristic
  for (auto& ca : candidates) {
    int combinedCandidateDist = ca->candidateDist();
//    logInfo() << "Dist of " << ca->getName() << " is " << combinedCandidateDist << " (" << ca->candidateDistA  << " and " << ca->candidateDistB << ")\n";
    int cappedDist = std::min(combinedCandidateDist, NumLCABuckets-1);
    candidateDistCount[cappedDist]++;
    bool consideredInHeuristic = type == CAHeuristicType::ALL;
    if (ca->isPartiallyDistinct) {
      candidateDistCountPartiallyDistinct[cappedDist]++;
      if (!consideredInHeuristic && type != CAHeuristicType::DISTINCT) {
        consideredInHeuristic = true;
      }
    }
    if (ca->isDistinct) {
      candidateDistCountDistinct[cappedDist]++;
//      if (ca->lcaDist > 1) {
//        logError() << "Candidate with LCA-Dist " << ca->lcaDist << " is marked as distinct!\n";
//        logError() << "SCC size: " << ca->node->size() << "\n";
//      }
      numDistinct++;
      consideredInHeuristic = true;
    }
    if (consideredInHeuristic /*&& combinedCandidateDist <= maxLCADist*/) {
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

  bool printCandidateStats = checkVerbosity(LOG_EXTRA);

  // Print stats
  if (printCandidateStats) {

    // DEBUG
    for (auto &ca : candidates) {
      if (ca->candidateDist() == 1 && ca->isDistinct) {
        logInfo() << "Candidate " << *ca << " of distance 1 is distinct.\n";
        auto subgraph = std::make_unique<CallGraph>();
        DecorationMap decoMap;
        makeReachableSubgraph(*subgraph, ca, decoMap);
        auto fileName = ca->getName() + ".dot";
        std::ofstream out(fileName);
        writeDOT(*subgraph, {}, decoMap, out);

        for (auto &child : sccResults.findAllCallees(ca->node)) {
          const auto &childData = sccDataMap[child];
          if (childData.isCA())
            continue;
          if (childData.reachesA())
            logInfo() << "--> Child " << childData.getName() << " reaches only A.\n";
          else if (childData.reachesB())
            logInfo() << "--> Child " << childData.getName() << " reaches only B.\n";
        }
        break;
      }
    }

    for (int i = 0; i < NumLCABuckets; i++) {

      std::deque<CommonCallerSearchNodeDataSCC*> candidateQ;
      std::deque<CommonCallerSearchNodeDataSCC*> partiallyDistinctQ;
      std::deque<CommonCallerSearchNodeDataSCC*> distinctQ;

      for (auto &ca : candidates) {

        int cappedDist = std::min(ca->candidateDist(), NumLCABuckets - 1);
        if (cappedDist <= i) {
          candidateQ.push_back(ca);
          if (ca->isPartiallyDistinct) {
            partiallyDistinctQ.push_back(ca);
            if (ca->isDistinct) {
              distinctQ.push_back(ca);
            }
          }
        }
      }


      logInfo() << "Candidates with LCA-Dist <= " << i << ": " << candidateQ.size()
                << std::endl;
//      logInfo() << dumpNodeSet("Candidates", candidateQ) << "\n";
      auto instrCandidates = getInstrumented(candidateQ);
      logInfo() << " -> instrumented functions: " << instrCandidates.size() << std::endl;

      logInfo() << "  of these partially distinct: "
                << partiallyDistinctQ.size() << std::endl;
      auto instrPartiallyDistinctCandidates = getInstrumented(partiallyDistinctQ);
      logInfo() << "  -> instrumented functions: " << instrPartiallyDistinctCandidates.size() << std::endl;

      logInfo() << "  of these fully distinct: "
                << distinctQ.size() << std::endl;
      auto instrDistinctCandidates = getInstrumented(distinctQ);
      logInfo() << "   -> instrumented functions: " << instrDistinctCandidates.size() << std::endl;
    }

    logInfo() << "------------------------------" << std::endl;

    for (int i = 0; i < NumLCABuckets - 1; i++) {
      logInfo() << "Candidates with LCA-Dist = " << i << ": " << candidateDistCount[i]
                << std::endl;

      logInfo() << "  of these partially distinct: "
                << candidateDistCountPartiallyDistinct[i] << std::endl;
      logInfo() << "  of these distinct: "
                << candidateDistCountDistinct[i] << std::endl;
    }
    logInfo() << "Candidates with LCA-Dist >= " << NumLCABuckets - 1 << ": "
              << candidateDistCount.back() << std::endl;
    logInfo() << "  of these partially distinct: "
              << candidateDistCountPartiallyDistinct.back() << std::endl;
    logInfo() << "  of these distinct: "
              << candidateDistCountDistinct.back() << std::endl;

    // Number of instrumented function for each bucket

  } else {
    LOG_STATUS("Number of candidate dLCAs with LCADist <= " << maxLCADist << ": " << workQueue.size() << "\n");
  }

  FunctionSet out = getInstrumented(workQueue);

  return out;
}

}
