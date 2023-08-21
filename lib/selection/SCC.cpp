//
// Created by sebastian on 10.07.23.
//

#include "SCC.h"

namespace capi {

struct SCCData {
  const CGNode* node{nullptr};
  int index{-1};
  int lowlink{-1};
  bool onStack{false};

  inline bool undefined() const {
    return index < 0;
  }
};


static void strongConnect(std::unordered_map<const CGNode*, SCCData>& sccMap, bool followVirtualCall, std::vector<SCCData*>& nodeStack, int& index, SCCData& nodeData, std::vector<SCCNode>& sccs) {
  nodeData.index = index;
  nodeData.lowlink = index;
  index++;
  nodeStack.push_back(&nodeData);
  nodeData.onStack = true;
  for (auto& callee : followVirtualCall ? nodeData.node->findAllCallees() : nodeData.node->getCallees()) {
    auto& calleeData = sccMap[callee];
    if (calleeData.undefined()) {
      calleeData.node = callee;
      strongConnect(sccMap, followVirtualCall, nodeStack, index, calleeData, sccs);
      nodeData.lowlink = std::min(nodeData.lowlink, calleeData.lowlink);
    } else if (calleeData.onStack) {
      nodeData.lowlink = std::min(nodeData.lowlink, calleeData.index);
    }
  }
  if (nodeData.lowlink == nodeData.index) {
    std::vector<const CGNode*> scc;
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
SCCAnalysisResults computeSCCs(const CallGraph& cg, bool followVirtualCalls) {
  std::vector<SCCNode> sccs;
  std::unordered_map<const CGNode*, SCCData> sccMap;
  std::vector<SCCData*> nodeStack;
  int index = 0;

  for (const auto& node : cg.getNodes()) {
    auto& nodeData = sccMap[node.get()];
    if (nodeData.undefined()) {
      nodeData.node = node.get();
      strongConnect(sccMap, followVirtualCalls, nodeStack, index, nodeData, sccs);
    }
  }

  return SCCAnalysisResults(sccs);
}

}