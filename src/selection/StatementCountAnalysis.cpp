//
// Created by sebastian on 02.04.25.
//

#include "capi/selection/StatementCountAnalysis.h"

#include <vector>
#include <queue>

#include "capi/support/Logging.h"
#include "metadata/NumStatementsMD.h"

namespace capi {

using namespace metacg;

static long computeInclusiveStatementCount(const CgNode* node, TraversalHelper& helper, const std::unordered_map<size_t, long>& cache) {
  std::vector<const CgNode*> workQueue;
  workQueue.push_back(node);
  std::unordered_set<const CgNode*> visitedNodes;
  unsigned long inclusiveStmtCount = 0;

  while (!workQueue.empty()) {
    auto wnode = workQueue.back();
    if (!wnode->has<NumStatementsMD>()) {
      logError() << "Need NumStatementsMD to compute inclusive statement count\n";
      return -1;
    }
    const auto numStatementsMD = wnode->get<NumStatementsMD>();
    workQueue.pop_back();
    if (visitedNodes.find(wnode) == visitedNodes.end()) {
      visitedNodes.insert(wnode);

      if (auto it = cache.find(wnode->getId()); it != cache.end()) {
        inclusiveStmtCount += it->second;
        continue;
      }

      const unsigned long perNodeStmtCount = numStatementsMD->getNumberOfStatements();
      inclusiveStmtCount += perNodeStmtCount;

      for (const auto& childNode : helper.get(wnode).findAllCallees()) {
        workQueue.push_back(childNode);
      }
    }
  }
  return inclusiveStmtCount;
}

bool StatementCountAnalysis::run(TraversalHelper& helper) {
  std::unordered_map<size_t, long> cache;

  std::queue<const CgNode*> workQueue;
  std::unordered_set<const CgNode*> visitedNodes;

  for (auto& leaf :  helper.findLeaves()) {
    workQueue.push(leaf);
  }

  while (!workQueue.empty()) {
    auto* node = workQueue.front();
    workQueue.pop();
    visitedNodes.insert(node);
    long isc = computeInclusiveStatementCount(node, helper, cache);
    if (isc < 0) {
      return false;
    }
    cache[node->getId()] = isc;
    for (auto* caller : helper.get(node).findAllCallers()) {
      if (visitedNodes.find(caller) == visitedNodes.end()) {
        workQueue.push(caller);
      }
    }
  }

  for (auto& [id, isc] : cache) {
    auto* node = helper.cg.getNode(id);
    assert(node && "Node should not be null here");
    node->getOrCreate<ISCMD>().value = isc;
  }
  return true;
}

}