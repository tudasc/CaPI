//
// Created by sebastian on 01.04.25.
//

#include "capi/selection/TraversalHelper.h"
#include "metadata/OverrideMD.h"

namespace capi {

void NodeTraversalInfo::compute(const metacg::CgNode& node, TraversalHelper* helper) {
  this->node = &node;
  this->helper = helper;
  this->isDestructor = isNodeDestructor(node);
  auto callees = helper->cg.getCallees(node);
  this->callees.insert(callees.begin(), callees.end());
  auto callers = helper->cg.getCallers(node);
  this->callers.insert(callers.begin(), callers.end());

  // FIXME: Assuming for now that all function calls are virtual
  virtualCalls = this->callees;
  virtualCalledBy = this->callers;
  if (!node.has<metacg::OverrideMD>()) {
    return;
  }
  auto overrideMD = node.get<metacg::OverrideMD>();
  overrides = overrideMD->overrides;
  overriddenBy = overrideMD->overriddenBy;
}

void NodeTraversalInfo::updateOverridesCache() {
  if (overridesComputed) {
    return;
  }
  // Update overrides
  recursiveOverrides.clear();
  for (const auto& overridesNode : this->overrides) {
    auto overridesNodeTraversalInfo = helper->get(overridesNode);
    recursiveOverrides.insert(overridesNodeTraversalInfo->node);
    assert(overridesNodeTraversalInfo && "Node must exist here");
    auto recursiveNodeOverrides = overridesNodeTraversalInfo->findAllOverrides();
    recursiveOverrides.insert(recursiveNodeOverrides.begin(), recursiveNodeOverrides.end());
  }
  overridesComputed = true;
}

void NodeTraversalInfo::updateOverriddenByCache()  {
  if (overriddenByComputed) {
    return;
  }
  // Update overriddenBy
  recursiveOverriddenBy.clear();
  for (const auto& overriddenByNode : this->overriddenBy) {
    auto overriddenByNodeTraversalInfo = helper->get(overriddenByNode);
    assert(overriddenByNodeTraversalInfo && "Node must exist here");
    recursiveOverriddenBy.insert(overriddenByNodeTraversalInfo->node);
    auto recursiveNodeOverridenBy = overriddenByNodeTraversalInfo->findAllOverriddenBy();
    recursiveOverriddenBy.insert(recursiveNodeOverridenBy.begin(), recursiveNodeOverridenBy.end());
  }
  overriddenByComputed = true;
}

void NodeTraversalInfo::updateAllCallersCache() {
  if (callersComputed) {
    return;
  }
  allCallers.clear();
  allCallers.insert(callers.begin(), callers.end());
  if (helper->shouldTraverseVirtualDtors() && !isDestructor) {
    for(auto& overrides: findAllOverrides()) {
      auto overridesCache = helper->get(overrides);
      allCallers.insert(overridesCache.virtualCalledBy.begin(), overridesCache.virtualCalledBy.end());
    }
  }
  callersComputed = true;
}

void NodeTraversalInfo::updateAllCalleesCache() {
  if (calleesComputed) {
    return;
  }
  allCallees.clear();
  allCallees.insert(callees.begin(), callees.end());
  for (auto* callee: virtualCalls) {
    auto& calleeCache = helper->get(callee);
    if (!helper->shouldTraverseVirtualDtors() || calleeCache.isDestructor) {
      continue;
    }
    auto allOverriddenBy = calleeCache.findAllOverriddenBy();
    allCallees.insert(allOverriddenBy.begin(), allOverriddenBy.end());
  }

  calleesComputed = true;
}

}