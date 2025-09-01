//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_SELECTORGRAPH_H
#define CAPI_SELECTORGRAPH_H


#include "Selector.h"

#include <unordered_map>
#include <utility>

namespace capi {

class PipelineNode {
  std::string name;
  std::vector<SelectorPtr> selectors;
  std::vector<std::string> inputs;
 public:

  PipelineNode(std::string name, std::vector<SelectorPtr> selectors) : name(std::move(name)), selectors(std::move(selectors)) {

  }

  const std::string& getName() const {
    return name;
  }

  void setName(const std::string& name) {
    this->name = name;
  }

  std::vector<SelectorPtr>& getSelectors() {
    return selectors;
  }

  void addInputDependency(std::string dep) {
    inputs.push_back(std::move(dep));
  }

  std::vector<std::string>& getInputDependencies() {
    return inputs;
  }

  size_t getSize() { return selectors.size(); }
};

using PipelineNodePtr = std::unique_ptr<PipelineNode>;

using SelectionResults = std::unordered_map<std::string, FunctionSet>;


class SelectorGraph {

  std::unordered_map<std::string, PipelineNodePtr> nodes;

  std::unordered_set<std::string> entryNodeNames;

public:
  SelectorGraph() = default;

  PipelineNode* getNode(const std::string& name) {
    auto it = nodes.find(name);
    if (it != nodes.end()) {
      return it->second.get();
    }
    return nullptr;
  }

  const PipelineNode* getNode(const std::string& name) const {
    auto it = nodes.find(name);
    if (it != nodes.end()) {
      return it->second.get();
    }
    return nullptr;
  }

  PipelineNode* createNode(const std::string& name, std::vector<SelectorPtr> selectors) {
    nodes[name] = std::make_unique<PipelineNode>(name, std::move(selectors));
    return nodes[name].get();
  }

  void replaceUses(const std::string& original, const std::string& replacement) {
    for (auto& [id, node]: nodes) {
      auto& deps = node->getInputDependencies();
      auto it = deps.begin();
      while ((it = std::find(it, deps.end(), original)) != deps.end()) {
        *it = replacement;
        ++it;
      }
    }
  }

  void eraseUnreachable() {
    std::unordered_set<std::string> reachable;
    std::unordered_set<std::string> workList;
    workList.insert(entryNodeNames.begin(), entryNodeNames.end());
    while (!workList.empty()) {
      auto& item = *workList.begin();
      reachable.insert(item);
      auto* node = getNode(item);
      for (auto& inputDep : node->getInputDependencies()) {
        workList.insert(inputDep);
      }
      workList.erase(item);
    }
    std::erase_if(nodes, [&reachable](auto& item) -> bool {
      bool erase = std::find(reachable.begin(), reachable.end(), item.first) == reachable.end();
      if (erase) {
      }
      return erase;
    });
  }

  void addEntryNode(std::string name) {
    entryNodeNames.insert(std::move(name));
  }

  std::vector<PipelineNode*> getEntryNodes()  {
    std::vector<PipelineNode*> entryNodes;
    for (auto& name : entryNodeNames) {
      auto n = getNode(name);
      if (n) {
        entryNodes.push_back(n);
      }
    }
    return entryNodes;
  }

  bool hasNode(const std::string& name) {
    return getNode(name) != nullptr;
  }

  const decltype(nodes)& getNodes() const {
    return nodes;
  }
};

using SelectorGraphPtr = std::unique_ptr<SelectorGraph>;

SelectionResults runSelectorPipeline(SelectorGraph& graph, TraversalHelper& helper, bool debugMode);

void dumpSelectorGraph(std::ostream& os, SelectorGraph& graph);

void dumpSelection(std::ostream& os, FunctionSet& functions);

}

#endif // CAPI_SELECTORGRAPH_H
