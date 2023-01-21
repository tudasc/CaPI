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
  FunctionInfo info;

public:
  CGNode(std::string name) : name(name)
  {}

  void setFunctionInfo(FunctionInfo fi)
  { this->info = fi; }

  FunctionInfo &getFunctionInfo()
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
