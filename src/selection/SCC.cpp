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
    auto& nodeData = sccMap[node.get()];
    if (nodeData.undefined()) {
      nodeData.node = node.get();
      strongConnect(sccMap, helper, followVirtualCalls, nodeStack, index, nodeData, sccs);
    }
  }

  return SCCAnalysisResults(sccs);
}

}