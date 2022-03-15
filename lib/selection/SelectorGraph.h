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

class SelectorGraph {

  std::unordered_map<std::string, SelectorNodePtr> nodes;

  std::string entryNodeName;

public:
  SelectorGraph() = default;

  SelectorNode* getNode(const std::string& name) {
    auto it = nodes.find(name);
    if (it != nodes.end()) {
      return it->second.get();
    }
    return nullptr;
  }

  SelectorNode* createNode(const std::string& name, SelectorPtr selector) {
    nodes[name] = std::make_unique<SelectorNode>(name, std::move(selector));
  }

  void setEntryNode(std::string name) {
    this->entryNodeName = std::move(name);
  }

  SelectorNode* getEntryNode() {
    return getNode(entryNodeName);
  }

  bool hasNode(const std::string& name) {
    return getNode(name) != nullptr;
  }
};

using SelectorGraphPtr = std::unique_ptr<SelectorGraph>;

FunctionSet runSelectorPipeline(SelectorGraph& graph, CallGraph &cg);

}

#endif // CAPI_SELECTORGRAPH_H
