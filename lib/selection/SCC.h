//
// Created by sebastian on 10.07.23.
//

#ifndef CAPI_SCC_H
#define CAPI_SCC_H

#include "CallGraph.h"

namespace capi {

struct SCCAnalysisResults {

  SCCAnalysisResults() = default;
  explicit SCCAnalysisResults(std::vector<std::vector<const CGNode*>> sccs) : sccs(std::move(sccs))
  {
    for (auto& scc : this->sccs) {
      for (auto& node: scc) {
        nodeMap[node] = &scc;
      }
    }
  }

  std::vector<std::vector<const CGNode*>> sccs;

  std::unordered_map<const CGNode*, std::vector<const CGNode*>*> nodeMap;

  size_t size() const {
    return sccs.size();
  }

  const std::vector<const CGNode*>* getSCC(const CGNode& node) const {
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

};

SCCAnalysisResults computeSCCs(const capi::CallGraph&, bool followVirtualCalls);

}

#endif // CAPI_SCC_H
