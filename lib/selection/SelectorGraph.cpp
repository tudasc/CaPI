//
// Created by sebastian on 15.03.22.
//

#include <cassert>
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
      if (std::find(workingSet.begin(), workingSet.end(), input) == workingSet.end()) {
        workingSet.insert(workingSet.begin(), input);
      }

    }
  }

//  std::cout << "Execution order: \n";
//  for (auto it = executionOrder.rbegin(); it != executionOrder.rend(); ++it) {
//    std::cout << "<" << (*it)->getSelector()->getName() << "> " << (*it)->getName() << ", ";
//  }
//  std::cout << "\n";


  std::unordered_map<std::string, FunctionSet> resultsMap;

  for (auto it = executionOrder.rbegin(); it != executionOrder.rend(); ++it) {
    auto node = *it;
    auto& selector = *node->getSelector();
    logInfo() << "Running selector '" << node->getName() << "' of type " << selector.getName() << "...\n";
    selector.init(cg);
    FunctionSetList inputList;
    for (auto&& inputName : node->getInputDependencies()) {
      auto resultIt = resultsMap.find(inputName);
      assert(resultIt != resultsMap.end() && "Invalid selector execution order");
      inputList.push_back(resultIt->second);
    }
    auto output = selector.apply(inputList);
    resultsMap[node->getName()] = std::move(output);
  }

  return resultsMap[selectorGraph.getEntryNode()->getName()];

}

void dumpSelectorGraph(std::ostream& os, SelectorGraph& graph) {
  for (auto&& [name, nodePtr] : graph.getNodes()) {
    auto& deps = nodePtr->getInputDependencies();
    os << name;
    if (!deps.empty())
      os << " <- ";
    for (auto it = deps.begin(); it != deps.end(); ++it) {
      os << *it << (it+1 == deps.end() ? "" : ", ");
    }
    if (graph.getEntryNode() == nodePtr.get()) {
      os << " [Entry]";
    }
    os << "\n";
  }
}

}