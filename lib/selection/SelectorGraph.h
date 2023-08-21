//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_SELECTORGRAPH_H
#define CAPI_SELECTORGRAPH_H


#include "Selector.h"

#include <unordered_map>

namespace capi {

class SelectorNode {
  std::string name;
  SelectorPtr selector;
  std::vector<std::string> inputs;
public:

  SelectorNode(std::string name, SelectorPtr selector) : name(name), selector(std::move(selector)) {

  }

  std::string getName() const {
    return name;
  }

  Selector* getSelector() {
    return selector.get();
  }

  void addInputDependency(std::string dep) {
    inputs.push_back(std::move(dep));
  }

  std::vector<std::string>& getInputDependencies() {
    return inputs;
  }


};

using SelectorNodePtr = std::unique_ptr<SelectorNode>;

using SelectionResults = std::unordered_map<std::string, FunctionSet>;


class SelectorGraph {

  std::unordered_map<std::string, SelectorNodePtr> nodes;

  std::unordered_set<std::string> entryNodeNames;

public:
  SelectorGraph() = default;

  SelectorNode* getNode(const std::string& name) {
    auto it = nodes.find(name);
    if (it != nodes.end()) {
      return it->second.get();
    }
    return nullptr;
  }

  const SelectorNode* getNode(const std::string& name) const {
    auto it = nodes.find(name);
    if (it != nodes.end()) {
      return it->second.get();
    }
    return nullptr;
  }

  SelectorNode* createNode(const std::string& name, SelectorPtr selector) {
    nodes[name] = std::make_unique<SelectorNode>(name, std::move(selector));
    return nodes[name].get();
  }

  void addEntryNode(std::string name) {
    entryNodeNames.insert(std::move(name));
  }

  std::vector<SelectorNode*> getEntryNodes()  {
    std::vector<SelectorNode*> entryNodes;
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

SelectionResults runSelectorPipeline(SelectorGraph& graph, CallGraph &cg, bool debugMode);

void dumpSelectorGraph(std::ostream& os, SelectorGraph& graph);

void dumpSelection(std::ostream& os, FunctionSet& functions);

}

#endif // CAPI_SELECTORGRAPH_H
