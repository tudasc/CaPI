//
// Created by sebastian on 28.10.21.
//

#ifndef CAPI_CALLGRAPH_H
#define CAPI_CALLGRAPH_H

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "MetaCGReader.h"

namespace capi {

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
  FunctionInfo info;

  // Cache
  mutable std::vector<CGNode*> recursiveOverrides;
  mutable bool overridesCacheDirty{true};
  mutable std::vector<CGNode*> recursiveOverriddenBy;
  mutable bool overriddenByCacheDirty{true};

public:
  // FIXME: This is only a temporary solution to store additional information for the filter file!
  // Remove ASAP!
  mutable bool isTrigger{false};

  explicit CGNode(std::string name) : name(std::move(name))
  {}

  void setFunctionInfo(FunctionInfo fi)
  { this->info = std::move(fi); }

  const FunctionInfo &getFunctionInfo() const
  { return info; }

  const std::string &getName() const
  { return name; }

  void addCallee(CGNode *callee)
  { callees.push_back(callee); }

  void removeCallee(CGNode *callee)
  {
    callees.erase(std::remove(callees.begin(), callees.end(), callee),
                  callees.end());
  }

  void addCaller(CGNode *caller)
  { callers.push_back(caller); }

  void removeCaller(CGNode *caller)
  {
    callers.erase(std::remove(callers.begin(), callers.end(), caller),
                  callers.end());
  }
  void addOverridenBy(CGNode *overriding)
  {
    overriddenBy.push_back(overriding);
    overriddenByCacheDirty = true;
  }

  void removeOverridenBy(CGNode *overriding)
  {
    overriddenBy.erase(std::remove(overriddenBy.begin(), overriddenBy.end(), overriding),
        overriddenBy.end());
    overriddenByCacheDirty = true;
  }
  void addOverrides(CGNode *overridesNode)
  {
    overrides.push_back(overridesNode);
    overridesCacheDirty = true;
  }

  void removeOverrides(CGNode *overridesNode)
  {
    overrides.erase(std::remove(overrides.begin(), overrides.end(), overridesNode),
                      overrides.end());
    overridesCacheDirty = true;
  }

  IterRange<decltype(callees.begin())> getCallees()
  {
    return IterRange(callees.begin(), callees.end());
  }

  IterRange<decltype(callers.begin())> getCallers()
  {
    return IterRange(callers.begin(), callers.end());
  }

  IterRange<decltype(callees.cbegin())> getCallees() const
  {
    return IterRange(callees.cbegin(), callees.cend());
  }

  IterRange<decltype(callers.cbegin())> getCallers() const
  {
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

  void addCallee(CGNode &parent, CGNode &child);

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
