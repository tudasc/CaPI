//
// Created by sebastian on 10.07.23.
//

#include "capi/selection/SCC.h"

#include <queue>

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

  std::unordered_map<const SCCNode*, std::vector<const SCCNode*>> res;
  std::unordered_map<const SCCNode*, int> inDegree;  // #parents not yet processed

  for (const SCCNode& node : sccs) {
    if (!inDegree.contains(&node)) {
      inDegree[&node] = 0;
    }

    for (const SCCNode* parent : findAllCallers(&node, helper)) {
      inDegree[&node]++;
    }
  }

  // Initialize working queue with root nodes (= no parents)
  std::queue<const SCCNode*> q;
  for (auto& [node, deg] : inDegree) {
    if (deg == 0) {
      q.push(node);
    }
  }

  res.reserve(inDegree.size());

  // Populate ancestor lists using Kahn's algorithm
  while (!q.empty()) {
    const SCCNode* node = q.front();
    q.pop();

    // Every parent is guaranteed to be in `res` already.
    // Collect ancestors: direct parents + all of their ancestor sets.
    std::vector<const SCCNode*>& ancestors = res[node];
    for (const SCCNode* parent : findAllCallers(node, helper)) {
      ancestors.push_back(parent);

      const auto& pa = res.at(parent);
      ancestors.insert(ancestors.end(), pa.begin(), pa.end());
    }

    // Deduplication 
    std::sort(ancestors.begin(), ancestors.end());
    ancestors.erase(std::unique(ancestors.begin(), ancestors.end()), ancestors.end());

    // Decrement inDegree counter in children
    for (const SCCNode* child : findAllCallees(node, helper)) {
      if (--inDegree[child] == 0) {
        q.push(child); // all parents processed -> child is ready
      }
    }
  }

  return res;
}
}