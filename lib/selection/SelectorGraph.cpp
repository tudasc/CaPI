//
// Created by sebastian on 15.03.22.
//

#include "SelectorGraph.h"
#include "Utils.h"

namespace capi {

FunctionSet runSelectorPipeline(SelectorGraph &selectorGraph, CallGraph &cg) {
  auto entry = selectorGraph.getEntryNode();
  if (!entry) {
    logError() << "No entry node specified!\n";
    return {};
  }

  std::vector<SelectorNode*> executionOrder;
  std::vector<SelectorNode*> workingSet;

  workingSet.push_back(entry);

  while (!workingSet.empty()) {
    auto node = workingSet.back();
    workingSet.pop_back();
    executionOrder.push_back(node);
    for (auto&& inputName : node->getInputDependencies()) {
      auto input = selectorGraph.getNode(inputName);
      if (!input) {
        logError() << "Invalid selector node: " << inputName << "\n";
        return {};
      }
      if (std::find(executionOrder.begin(), executionOrder.end(), input) != executionOrder.end()) {
        logError() << "Cyclic dependency!\n";
        return {};
      }
      if (std::find(workingSet.begin(), workingSet.end(), input) != workingSet.end()) {
        workingSet.insert(workingSet.begin(), input);
      }

    }
  }

  std::cout << "Execution order: \n";
  for (auto it = executionOrder.rbegin(); it != executionOrder.rend(); ++it) {
    std::cout << "<" << (*it)->getSelector()->getName() << "> " << (*it)->getName() << ", ";
  }
  std::cout << "\n";

  // TODO
  return {};

}

}