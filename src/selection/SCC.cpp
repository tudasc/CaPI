//
// Created by sebastian on 10.07.23.
//

#include "capi/selection/SCC.h"

namespace capi {

struct SCCData {
  const metacg::CgNode* node{nullptr};
  int index{-1};
  int lowlink{-1};
  bool onStack{false};

  inline bool undefined() const {
    return index < 0;
  }
};


static void strongConnect(std::unordered_map<const metacg::CgNode*, SCCData>& sccMap, capi::TraversalHelper& helper, bool followVirtualCall, std::vector<SCCData*>& nodeStack, int& index, SCCData& nodeData, std::vector<SCCNode>& sccs) {
  nodeData.index = index;
  nodeData.lowlink = index;
  index++;
  nodeStack.push_back(&nodeData);
  nodeData.onStack = true;
  for (auto& callee : followVirtualCall ? helper.get(nodeData.node).findAllCallees() :  helper.get(nodeData.node).getCallees()) {
    auto& calleeData = sccMap[callee];
    if (calleeData.undefined()) {
      calleeData.node = callee;
      strongConnect(sccMap, helper, followVirtualCall, nodeStack, index, calleeData, sccs);
      nodeData.lowlink = std::min(nodeData.lowlink, calleeData.lowlink);
    } else if (calleeData.onStack) {
      nodeData.lowlink = std::min(nodeData.lowlink, calleeData.index);
    }
  }
  if (nodeData.lowlink == nodeData.index) {
    std::vector<const metacg::CgNode*> scc;
    SCCData* member{nullptr};
    do {
      member = nodeStack.back();
      assert(member && "Nullptr on the stack");
      nodeStack.pop_back();
      member->onStack = false;
      scc.push_back(member->node);
    } while(member->node != nodeData.node);
    sccs.push_back({scc});
  }
}

// Implements Tarjan's algorithm
SCCAnalysisResults computeSCCs(capi::TraversalHelper& helper, bool followVirtualCalls) {
  std::vector<SCCNode> sccs;
  std::unordered_map<const metacg::CgNode*, SCCData> sccMap;
  std::vector<SCCData*> nodeStack;
  int index = 0;

  for (const auto& node : helper.cg.getNodes()) {
    if (node.get() == nullptr) {
      continue;
    }
    auto& nodeData = sccMap[node.get()];
    if (nodeData.undefined()) {
      nodeData.node = node.get();
      strongConnect(sccMap, helper, followVirtualCalls, nodeStack, index, nodeData, sccs);
    }
  }

  return SCCAnalysisResults(sccs);
}

std::unordered_map<const SCCNode*, std::vector<const SCCNode*>> SCCAnalysisResults::globalAncestorComputation(
    const std::unordered_set<const SCCNode*>& leafes, TraversalHelper& helper) {
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
  worklist.insert(worklist.end(), leafes.begin(), leafes.end());

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
      // res[node] = {};
      // std::cout << "Processed " << ++processed << " (worklist size: " << worklist.size() << ")\n";
    }
  }

  return res;
}
}