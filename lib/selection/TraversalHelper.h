//
// Created by sebastian on 01.04.25.
//

#ifndef CAPI_TRAVERSALHELPER_H
#define CAPI_TRAVERSALHELPER_H

#include "Callgraph.h"
#include "../support/IteratorUtils.h"

namespace capi {


inline bool isNodeDestructor(const metacg::CgNode& node) {
  auto name = node.getFunctionName();
  return name.substr(0, 2) == "_Z" && !(name.compare(name.length() - 4, 4, "D0Ev")
                                                    && name.compare(name.length() - 4, 4, "D1Ev")
                                                    && name.compare(name.length() - 4, 4, "D2Ev"));
}

template<typename T1, typename T2>
void insertIds(T1& container, T2& nodes) {
  for (auto& node : nodes) {
    container.push_back(node->getId());
  }
}

using ConstCgNodePtrSet = std::unordered_set<const metacg::CgNode*>;

struct TraversalHelper;

struct NodeTraversalInfo {
  TraversalHelper* helper{nullptr};
  const metacg::CgNode* node{nullptr};
  ConstCgNodePtrSet callees;
  ConstCgNodePtrSet callers;
  ConstCgNodePtrSet virtualCalls;
  ConstCgNodePtrSet virtualCalledBy;
  std::vector<size_t> overrides;
  std::vector<size_t> overriddenBy;

  bool overridesComputed{false};
  bool overriddenByComputed{false};
  bool callersComputed{false};
  bool calleesComputed{false};
  bool isDestructor{false};

  ConstCgNodePtrSet recursiveOverrides;
  ConstCgNodePtrSet recursiveOverriddenBy;
  ConstCgNodePtrSet allCallers;
  ConstCgNodePtrSet allCallees;

  void compute(const metacg::CgNode& node, TraversalHelper* helper);

  IterRange<decltype(callees.begin())> getCallees() {
    return IterRange(callees.begin(), callees.end());
  }

  IterRange<decltype(callers.begin())> getCallers() {
    return IterRange(callers.begin(), callers.end());
  }

  IterRange<decltype(callees.cbegin())> getCallees() const {
    return IterRange(callees.cbegin(), callees.cend());
  }

  IterRange<decltype(callers.cbegin())> getCallers() const {
    return IterRange(callers.cbegin(), callers.cend());
  }

  IterRange<decltype(overriddenBy.begin())> getOverriddenBy() {
    return {overriddenBy.begin(), overriddenBy.end()};
  }

  IterRange<decltype(overriddenBy.cbegin())> getOverriddenBy() const {
    return {overriddenBy.cbegin(), overriddenBy.cend()};
  }

  IterRange<decltype(overrides.begin())> getOverrides() {
    return {overrides.begin(), overrides.end()};
  }

  IterRange<decltype(overrides.cbegin())> getOverrides() const {
    return {overrides.cbegin(), overrides.cend()};
  }

  IterRange<decltype(recursiveOverrides.begin())> findAllOverrides() {
    updateOverridesCache();
    return {recursiveOverrides.begin(), recursiveOverrides.end()};
  }

  IterRange<decltype(recursiveOverriddenBy.cbegin())> findAllOverriddenBy() {
    updateOverriddenByCache();
    return {recursiveOverriddenBy.cbegin(), recursiveOverriddenBy.cend()};
  }

  IterRange<decltype(allCallers.begin())> findAllCallers() {
    updateAllCallersCache();
    return {allCallers.begin(), allCallers.end()};
  }

  IterRange<decltype(allCallers.begin())> findAllCallees() {
    updateAllCalleesCache();
    return {allCallees.begin(), allCallees.end()};
  }

private:
  void updateOverridesCache();

  void updateOverriddenByCache();

  void updateAllCallersCache();

  void updateAllCalleesCache();
};


struct TraversalHelper {
  metacg::Callgraph& cg;
  std::unordered_map<size_t, NodeTraversalInfo> nodeMap;

  bool traverseVirtualDtors;

  TraversalHelper(metacg::Callgraph& cg, bool traverseVirtualDtors) : cg(cg), traverseVirtualDtors(traverseVirtualDtors) {
  }

  NodeTraversalInfo & get(const metacg::CgNode* node) {
    auto& nodeCache = nodeMap[node->getId()];
    if (!nodeCache.node) {
      nodeCache.compute(*node, this);
    }
    return nodeCache;
  }

  NodeTraversalInfo * get(size_t id) {
    auto* node =  cg.getNode(id);
    if (!node) {
      return nullptr;
    }
    return &get(node);
  }

  std::vector<const metacg::CgNode*> findRoots() const {
    std::vector<const metacg::CgNode*> roots;
    for (auto& [id, node] : cg.getNodes()) {
      if (cg.getCallers(*node).empty())
        roots.push_back(node.get());
    }
    return roots;
  }

  std::vector<const metacg::CgNode*> findLeaves() const {
    std::vector<const metacg::CgNode*> leaves;
    for (auto& [id, node] : cg.getNodes()) {
      if (cg.getCallees(*node).empty())
        leaves.push_back(node.get());
    }
    return leaves;
  }

  bool shouldTraverseVirtualDtors() const {
    return traverseVirtualDtors;
  }

};



}

#endif // CAPI_TRAVERSALHELPER_H
