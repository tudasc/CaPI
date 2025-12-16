//
// Created by sebastian on 10.07.23.
//

#ifndef CAPI_SCC_H
#define CAPI_SCC_H

#include "capi/selection/TraversalHelper.h"
#include "capi/support/Logging.h"

// MetaCG includes
#include "Callgraph.h"
#include <unordered_set>

namespace capi {

struct SCCNode {
  std::vector<const metacg::CgNode*> nodes;

  size_t size() const {
    return nodes.size();
  }

  std::string getName() const {
    if (nodes.empty())
      return "EMPTY";
    return nodes.front()->getFunctionName() + "(" + std::to_string(size()) + ")";
  }
};

struct SCCAnalysisResults {

  SCCAnalysisResults() = default;
  explicit SCCAnalysisResults(std::vector<SCCNode> sccs) : sccs(std::move(sccs))
  {
    for (auto& scc : this->sccs) {
      for (auto& node: scc.nodes) {
        nodeMap[node] = &scc;
      }
    }
  }

  std::vector<SCCNode> sccs;

  std::unordered_map<const metacg::CgNode*, SCCNode*> nodeMap;

  size_t size() const {
    return sccs.size();
  }

  const SCCNode* getSCC(const metacg::CgNode& node) const {
    auto it = nodeMap.find(&node);
    if (it == nodeMap.end()) {
      logError() << "Node not found in SCC\n";
      return nullptr;
    }
    return it->second;
  }

  int getSCCSize(const metacg::CgNode& node) const {
    auto* scc = getSCC(node);
    return scc ? scc->size() : 0;
  }

  std::vector<const SCCNode*> findAllCallers(const SCCNode* node, TraversalHelper& helper) const {
    std::vector<const SCCNode*> callers;
    for (auto& v : node->nodes) {
      auto& vTInfo = helper.get(v);
      auto vCallers = vTInfo.findAllCallers();
      for (auto& w : vCallers)  {
        auto callerSCC = getSCC(*w);
        if (callerSCC == node)
          continue;
        if (std::find(callers.begin(), callers.end(), callerSCC) == callers.end()) {
          callers.push_back(callerSCC);
        }
      }
    }
    return callers;
  }

  std::vector<const SCCNode*> findAllCallees(const SCCNode* node, TraversalHelper& helper) const {
    std::vector<const SCCNode*> callees;
    for (auto& v : node->nodes) {
      auto& vTInfo = helper.get(v);
      auto vCallees = vTInfo.findAllCallees();
      for (auto& u : vCallees)  {
        auto calleeSCC = getSCC(*u);
        if (calleeSCC == node)
          continue;
        if (std::find(callees.begin(), callees.end(), calleeSCC) == callees.end()) {
          callees.push_back(calleeSCC);
        }
      }
    }
    return callees;
  }

  // Compute list of ancestors for each SCCNode. Exploits that SCC-graph is a DAG.
  std::unordered_map<const SCCNode*, std::vector<const SCCNode*>> globalAncestorComputation(
      const std::unordered_set<const SCCNode*>& leafes, TraversalHelper& helper);
};

// For graph trait
struct SCCGraph {
  const SCCAnalysisResults& sccResults;
  TraversalHelper& helper;

  explicit SCCGraph(const SCCAnalysisResults& sccAnalysisResults, TraversalHelper& helper) : sccResults(sccAnalysisResults), helper(helper) {}

  std::vector<const SCCNode*> getCallers(const SCCNode* node) const {
    return sccResults.findAllCallers(node, helper);
  }

  std::vector<const SCCNode*> getCallees(const SCCNode* node) const {
    return sccResults.findAllCallees(node, helper);
  }
};

SCCAnalysisResults computeSCCs(capi::TraversalHelper&, bool followVirtualCalls);

}

#endif // CAPI_SCC_H
