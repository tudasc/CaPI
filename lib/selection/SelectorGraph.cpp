//
// Created by sebastian on 15.03.22.
//

#include "SelectorGraph.h"
#include "../Utils.h"
#include <cassert>

namespace capi {

static bool dfsSort(SelectorNode* node, SelectorGraph& graph, std::vector<SelectorNode*>& execOrder, std::vector<SelectorNode*>& visited) {
  visited.push_back(node);
  for (auto&& inputName : node->getInputDependencies()) {
    auto input = graph.getNode(inputName);
    if (!input) {
      logError() << "Invalid selector node: " << inputName << "\n";
      return false;
    }
    if (std::find(visited.begin(), visited.end(), input) == visited.end()) {
      if (!dfsSort(input, graph, execOrder, visited)) {
        return false;
      }
    }
  }
  execOrder.push_back(node);
  return true;
}

FunctionSet runSelectorPipeline(SelectorGraph &selectorGraph, CallGraph &cg) {
  auto entry = selectorGraph.getEntryNode();
  if (!entry) {
    logError() << "No entry node specified!\n";
    return {};
  }

//  std::vector<SelectorNode*> executionOrder;
//  std::vector<SelectorNode*> workingSet;

//  workingSet.push_back(entry);
//
//  while (!workingSet.empty()) {
//    auto node = workingSet.back();
//    workingSet.pop_back();
//    executionOrder.push_back(node);
//    logError() << "Added " << node->getName() << "\n";
//    for (auto&& inputName : node->getInputDependencies()) {
//      auto input = selectorGraph.getNode(inputName);
//      if (!input) {
//        logError() << "Invalid selector node: " << inputName << "\n";
//        return {};
//      }
//      if (std::find(executionOrder.begin(), executionOrder.end(), input) != executionOrder.end()) {
//        logError() << "Cyclic dependency! " << input->getName() << "\n";
//        return {};
//      }
//      if (std::find(workingSet.begin(), workingSet.end(), input) == workingSet.end()) {
//        workingSet.insert(workingSet.begin(), input);
//      }
//
//    }
//  }


  std::vector<SelectorNode*> executionOrder;
  std::vector<SelectorNode*> visited;

  bool success = dfsSort(entry, selectorGraph, executionOrder, visited);

  if (!success) {
    logError() << "Invalid selector pipeline.\n";
    return {};
  }



//  std::cout << "Execution order: \n";
//  for (auto it = executionOrder.rbegin(); it != executionOrder.rend(); ++it) {
//    std::cout << "<" << (*it)->getSelector()->getName() << "> " << (*it)->getName() << ", ";
//  }
//  std::cout << "\n";


  std::unordered_map<std::string, FunctionSet> resultsMap;

  for (auto it = executionOrder.begin(); it != executionOrder.end(); ++it) {
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
    logInfo() << "Selector '" << node->getName() << "' selected " << output.size() << " functions.\n";
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