//
// Created by sebastian on 28.10.21.
//

#include "CallGraph.h"

#include <cassert>
#include <iostream>

namespace capi {

CGNode &CallGraph::getOrCreate(const std::string &name)
{
  auto it = nodeMap.find(name);
  if (it == nodeMap.end()) {
    auto idx = nodes.size();
    nodes.emplace_back(std::make_unique<CGNode>(name));
    nodeMap[name] = idx;
    return *nodes.back();
  }
  assert(it->second < nodes.size() && "Node map is corrupted");
  return *nodes[it->second];
}

CGNode *CallGraph::get(const std::string &name)
{
  auto it = nodeMap.find(name);
  if (it == nodeMap.end())
    return nullptr;
  assert(it->second < nodes.size() && "Node map is corrupted");
  return &*nodes[it->second];
}

// bool CallGraph::deleteNode(const std::string &name) {
//    auto it = nodeMap.find(name);
//    if (it == nodeMap.end())
//        return false;
//
//    for (auto& callee : )
//}

void CallGraph::addCallee(CGNode &parent, CGNode &child, bool isVirtual)
{
  parent.addCallee(&child, isVirtual);
  child.addCaller(&parent, isVirtual);
}

void CallGraph::removeCallee(CGNode &parent, CGNode &child)
{
  parent.removeCallee(&child);
  child.removeCaller(&parent);
}

void CallGraph::addOverrides(CGNode& node, CGNode& overrides) {
  node.addOverrides(&overrides);
  overrides.addOverridenBy(&node);
}

void CallGraph::removeOverrides(CGNode& node, CGNode& overrides) {
  node.removeOverrides(&overrides);
  overrides.removeOverridenBy(&node);
}

std::unique_ptr<CallGraph> createCG(FInfoMap &fInfoMap)
{
  auto cg = std::make_unique<CallGraph>();
  for (auto &[name, info] : fInfoMap) {
    auto &node = cg->getOrCreate(name);
    node.setFunctionInfo(info);
    auto& vCalls = info.virtualCalls;
    std::for_each(info.callees.begin(), info.callees.end(),
                  [&cg, &node, &vCalls](const std::string& callee) {
                    auto &calleeNode = cg->getOrCreate(callee);
                    bool isVirtual = std::find(vCalls.begin(), vCalls.end(), callee) != vCalls.end();
                    cg->addCallee(node, calleeNode, isVirtual);
                  });
    std::for_each(info.overrides.begin(), info.overrides.end(),
        [&cg, &node](const std::string& overridesName) {
          auto& overridesNode = cg->getOrCreate(overridesName);
          cg->addOverrides(node, overridesNode);
        });
  }
  return cg;
}

}