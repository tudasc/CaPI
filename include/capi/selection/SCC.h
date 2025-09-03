//
// Created by sebastian on 10.07.23.
//

#ifndef CAPI_SCC_H
#define CAPI_SCC_H

#include "capi/selection/TraversalHelper.h"
#include "capi/support/Logging.h"

// MetaCG includes
#include "Callgraph.h"

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

  std::unordered_map<const SCCNode*, std::vector<const SCCNode*>> globalAncestorComputation(TraversalHelper& helper) {
    // std::cout << "Total number of SCC nodes: " << sccs.size() << "\n";
    // unsigned processed = 0;
    
    // result map
    std::unordered_map<const SCCNode*, std::vector<const SCCNode*>> res;

    // Queue of nodes to be processed
    std::vector<const SCCNode*> worklist;

    // Temporary vector holding all parents of a node that have not been processed yet
    std::vector<const SCCNode*> missingParents;

    // Temporary vector holding the ancestors of all parents that have already been processed
    std::vector<std::vector<const SCCNode*>> parentAncestorsCollection;

    // Put all leaf SCCs into the worklist
    for (const metacg::CgNode* node : helper.findLeaves()) {
      worklist.push_back(getSCC(*node));
    }

    while (!worklist.empty()) {
      const SCCNode* node = worklist.back();
      worklist.pop_back();
      
      // Skip if this node was already processed
      if (res.contains(node)) {
        continue;
      }
      
      // Fresh temporary vectors
      missingParents.clear();
      parentAncestorsCollection.clear();
      
      auto parents = findAllCallers(node, helper);

      // iterate over all parents to popolate `missingParents` and `parentAncestorsCollection`
      for (const SCCNode* parent : parents) {
        auto parent_res = res.find(parent);

        if (parent_res == res.end()) {
          // parent has not been processed yet
          missingParents.push_back(parent);
        } else {
          // parent has already been processed -> save its ancestors
          parentAncestorsCollection.push_back((parent_res->second));
        }
      }

      // check whether all parents have already been processed
      if (!missingParents.empty()) {
        // there are still parents which have not yet been processed

        // re-visit this node after all parents have been visited
        // (thus push it onto the worklist first)
        worklist.push_back(node);

        // push all missing parents onto the worklist
        for (const SCCNode* parent : missingParents) {
          worklist.push_back(parent);
          // std::cout << node->getName() << ": pushing parent " << parent->getName() << "\n";
        }
        // std::cout << "Worklist size: " << worklist.size() << "\n";
      } else {
        // all parents have already been processed \o/

        // collect this node's ancestors (i.e., its parents and its parent's ancestors)
        std::vector<const SCCNode*> ancestors;
        for (const SCCNode* parent : parents) {
          ancestors.push_back(parent);
        }
        for (auto& parentAncestors : parentAncestorsCollection) {
          for (const SCCNode* parentAncestor : parentAncestors) {
            ancestors.push_back(parentAncestor);
          }
        }

        // save into result map
        res[node] = ancestors;
        // std::cout << "Processed " << ++processed << " (worklist size: " << worklist.size() << ")\n";
      }
    }

    return res;
  }

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
