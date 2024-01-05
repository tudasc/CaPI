//
// Created by sebastian on 10.07.23.
//

#ifndef CAPI_SCC_H
#define CAPI_SCC_H

#include "CallGraph.h"

namespace capi {

struct SCCNode {
  std::vector<const CGNode*> nodes;

  size_t size() const {
    return nodes.size();
  }

  std::string getName() const {
    if (nodes.empty())
      return "EMPTY";
    return nodes.front()->getName();
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

  std::unordered_map<const CGNode*, SCCNode*> nodeMap;

  size_t size() const {
    return sccs.size();
  }

  const SCCNode* getSCC(const CGNode& node) const {
    auto it = nodeMap.find(&node);
    if (it == nodeMap.end()) {
      std::cerr << "Node not found in SCC\n";
      return nullptr;
    }
    return it->second;
  }

  int getSCCSize(const CGNode& node) const {
    auto* scc = getSCC(node);
    return scc ? scc->size() : 0;
  }

  std::vector<const SCCNode*> findAllCallers(const SCCNode* node) const {
    std::vector<const SCCNode*> callers;
    for (auto& v : node->nodes) {
      auto vCallers = v->findAllCallers();
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

  std::vector<const SCCNode*> findAllCallees(const SCCNode* node) const {
    std::vector<const SCCNode*> callees;
    for (auto& v : node->nodes) {
      auto vCallees = v->findAllCallees();
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

};

// For graph trait
struct SCCGraph {
  const SCCAnalysisResults& sccResults;

  explicit SCCGraph(const SCCAnalysisResults& sccAnalysisResults) : sccResults(sccAnalysisResults) {}

  std::vector<const SCCNode*> getCallers(const SCCNode* node) const {
    return sccResults.findAllCallers(node);
  }

  std::vector<const SCCNode*> getCallees(const SCCNode* node) const {
    return sccResults.findAllCallees(node);
  }
};

SCCAnalysisResults computeSCCs(const capi::CallGraph&, bool followVirtualCalls);

}

#endif // CAPI_SCC_H
