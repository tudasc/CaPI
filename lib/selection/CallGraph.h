//
// Created by sebastian on 28.10.21.
//

#ifndef CAPI_CALLGRAPH_H
#define CAPI_CALLGRAPH_H

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "MetaCGReader.h"

namespace capi {

// Global option that is set during parameter parsing.
extern bool traverseVirtualDtors;

using FInfoMap = std::unordered_map<std::string, FunctionInfo>;

template<typename T>
struct IterRange
{
  IterRange(T begin, T end) : beginIt(begin), endIt(end)
  {}

  T begin()
  { return beginIt; }

  T end()
  { return endIt; }

  size_t size() const {
    return std::distance(beginIt, endIt);
  }

private:
  T beginIt, endIt;
};

class CGNode
{
  std::string name;
  std::vector<CGNode *> callers;
  std::vector<CGNode *> callees;
  std::vector<CGNode *> overrides;
  std::vector<CGNode *> overriddenBy;
  std::vector<CGNode *> virtualCalls;
  std::vector<CGNode *> virtualCalledBy;
  FunctionInfo info;
  bool destructor;

  // Cache
  mutable std::vector<CGNode*> recursiveOverrides;
  mutable bool overridesCacheDirty{true};
  mutable std::vector<CGNode*> recursiveOverriddenBy;
  mutable bool overriddenByCacheDirty{true};
  mutable std::vector<CGNode*> allCallers;
  mutable bool callersChanged{true};
  mutable std::vector<CGNode*> allCallees;
  mutable bool calleesChanged{true};

public:
  // FIXME: This is only a temporary solution to store additional information for the filter file!
  // Remove ASAP!
  mutable bool isTrigger{false};

  explicit CGNode(std::string name) : name(std::move(name))
  {
    destructor = this->name.substr(0, 2) == "_Z" && !(this->name.compare(this->name.length() - 4, 4, "D0Ev")
                   && this->name.compare(this->name.length() - 4, 4, "D1Ev")
                   && this->name.compare(this->name.length() - 4, 4, "D2Ev"));
  }

  void setFunctionInfo(FunctionInfo fi) {
    this->info = std::move(fi);
  }

  FunctionInfo &getFunctionInfo() {
    return info; 
}

  const FunctionInfo &getFunctionInfo() const {
    return info;
  }

  const std::string &getName() const {
    return name;
  }

  bool isDestructor() const {
    return destructor;
  }

  void addCallee(CGNode *callee, bool isVirtual) {
    callees.push_back(callee);
    if (isVirtual)
      virtualCalls.push_back(callee);
    calleesChanged = true;
  }

  void removeCallee(CGNode *callee) {
    callees.erase(std::remove(callees.begin(), callees.end(), callee),
                  callees.end());
    virtualCalls.erase(std::remove(virtualCalls.begin(), virtualCalls.end(), callee),
                  virtualCalls.end());
    calleesChanged = true;
  }

  void addCaller(CGNode *caller, bool isVirtual) {
    callers.push_back(caller);
    if (isVirtual)
      virtualCalledBy.push_back(caller);
    callersChanged = true;
  }

  void removeCaller(CGNode *caller) {
    callers.erase(std::remove(callers.begin(), callers.end(), caller),
                  callers.end());
    virtualCalledBy.erase(std::remove(virtualCalledBy.begin(), virtualCalledBy.end(), caller),
                       virtualCalledBy.end());
    callersChanged = true;
  }

  void addOverridenBy(CGNode *overriding) {
    overriddenBy.push_back(overriding);
    overriddenByCacheDirty = true;
  }

  void removeOverridenBy(CGNode *overriding) {
    overriddenBy.erase(std::remove(overriddenBy.begin(), overriddenBy.end(), overriding),
        overriddenBy.end());
    overriddenByCacheDirty = true;
  }
  void addOverrides(CGNode *overridesNode) {
    overrides.push_back(overridesNode);
    overridesCacheDirty = true;
  }

  void removeOverrides(CGNode *overridesNode) {
    overrides.erase(std::remove(overrides.begin(), overrides.end(), overridesNode),
                      overrides.end());
    overridesCacheDirty = true;
  }

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

  IterRange<decltype(recursiveOverrides.cbegin())> findAllOverrides() const{
    updateOverridesCache();
    return {recursiveOverrides.cbegin(), recursiveOverrides.cend()};
  }

  IterRange<decltype(recursiveOverriddenBy.cbegin())> findAllOverriddenBy() const{
    updateOverriddenByCache();
    return {recursiveOverriddenBy.cbegin(), recursiveOverriddenBy.cend()};
  }

  IterRange<decltype(allCallers.cbegin())> findAllCallers() const {
     updateAllCallersCache();
     return {allCallers.cbegin(), allCallers.cend()};
  }

  IterRange<decltype(allCallers.cbegin())> findAllCallees() const {
     updateAllCalleesCache();
     return {allCallees.cbegin(), allCallees.cend()};
  }

private:
  void updateOverridesCache() const {
    if (!overridesCacheDirty) {
      return;
    }
    // Update overrides
    recursiveOverrides.clear();
    for (const auto& overridesNode : this->overrides) {
      recursiveOverrides.push_back(overridesNode);
      auto recursiveNodeOverrides = overridesNode->findAllOverrides();
      recursiveOverrides.insert(recursiveOverrides.end(), recursiveNodeOverrides.begin(), recursiveNodeOverrides.end());
    }
    overridesCacheDirty = false;
  }

  void updateOverriddenByCache() const {
    if (!overriddenByCacheDirty) {
      return;
    }
    // Update overriddenBy
    recursiveOverriddenBy.clear();
    for (const auto& overriddenByNode : this->overriddenBy) {
      recursiveOverriddenBy.push_back(overriddenByNode);
      auto recursiveNodeOverridenBy = overriddenByNode->findAllOverriddenBy();
      recursiveOverriddenBy.insert(recursiveOverriddenBy.end(), recursiveNodeOverridenBy.begin(), recursiveNodeOverridenBy.end());
    }
    overriddenByCacheDirty = false;
  }

  void updateAllCallersCache() const {
    if (!callersChanged && !overridesCacheDirty) {
      return;
    }
    allCallers.clear();
    allCallers.insert(allCallers.end(), callers.begin(), callers.end());
    if (traverseVirtualDtors || !isDestructor()) {
      for(const auto *overrides: findAllOverrides()) {
        allCallers.insert(allCallers.end(),
                            overrides->virtualCalledBy.begin(),
                            overrides->virtualCalledBy.end());
      }
    }
  }

  void updateAllCalleesCache() const {
    if (!calleesChanged && !std::any_of(callees.begin(), callees.end(), [](auto* callee) {return callee->overriddenByCacheDirty;})) {
      return;
    }
    allCallees.clear();
    allCallees.insert(allCallees.end(), callees.begin(), callees.end());
    for (const auto* callee: virtualCalls) {
      if (!traverseVirtualDtors && callee->isDestructor()) {
        continue;
      }
      auto allOverriddenBy = callee->findAllOverriddenBy();
      allCallees.insert(allCallees.end(), allOverriddenBy.begin(), allOverriddenBy.end());
    }
  }

};

class CallGraph
{

  std::vector<std::unique_ptr<CGNode>> nodes;
  std::unordered_map<std::string, size_t> nodeMap;

public:
  CallGraph() = default;

  CallGraph(const CallGraph *) = delete;

  CallGraph &operator=(const CallGraph &) = delete;

  CallGraph(CallGraph &&) = default;

  CallGraph &operator=(CallGraph &&) = default;

  ~CallGraph() = default;

  size_t size() const
  { return nodes.size(); }

  CGNode &getOrCreate(const std::string &name);

  CGNode *get(const std::string &name);

  //    bool deleteNode(const std::string& name);

  void addCallee(CGNode &parent, CGNode &child, bool isVirtual);

  void removeCallee(CGNode &parent, CGNode &child);

  void addOverrides(CGNode& node, CGNode& overrides);

  void removeOverrides(CGNode& node, CGNode& overrides);

  IterRange<decltype(nodes.begin())> getNodes()
  {
    return IterRange(nodes.begin(), nodes.end());
  }

  IterRange<decltype(nodes.cbegin())> getNodes() const
  {
    return IterRange(nodes.cbegin(), nodes.cend());
  }

  std::vector<const CGNode*> findRoots() const {
    std::vector<const CGNode*> roots;
    for (auto& node : nodes) {
      if (node->getCallers().size() == 0)
        roots.push_back(node.get());
    }
    return roots;
  }

};

std::unique_ptr<CallGraph> createCG(FInfoMap &fInfoMap);

}

#endif // CAPI_CALLGRAPH_H
