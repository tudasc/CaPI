//
// Created by sebastian on 15.03.22.
//

#include "SelectorGraph.h"
#include "../Utils.h"
#include <cassert>
#include <fstream>
#include <omp.h>

namespace capi {

namespace {
  void processNode(SelectorNode *node, CallGraph &cg, std::unordered_map<std::string, FunctionSet>& resultsMap)
  {
    auto& selector = *node->getSelector();
    logInfo() << "Running selector '" << node->getName() << "' of type " << selector.getName() << ", Thread: " << omp_get_thread_num() << " ...\n";
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

  void processNodeHelper(SelectorNode *node,
                       CallGraph &cg,
                       std::unordered_map<std::string, FunctionSet>& resultsMap,
                       int index,
                       const std::vector<SelectorNode*>& executionOrder,
                       const std::unordered_map<std::string, int>& nodeOrderMap)
  {
    if (node->getInputDependencies().size() > index)
    {
      // wait on node in index
      #pragma omp task depend(in: executionOrder[nodeOrderMap.at(node->getInputDependencies()[index])]) shared(resultsMap,cg,executionOrder)
      processNodeHelper(node, cg, resultsMap, index + 1, executionOrder, nodeOrderMap);
    }
    else
    {
      processNode(node, cg, resultsMap);
    }

    #pragma omp taskwait
  }
}

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

FunctionSet runSelectorPipeline(SelectorGraph &selectorGraph, CallGraph &cg, bool debugMode) {
  auto entry = selectorGraph.getEntryNode();
  if (!entry) {
    logError() << "No entry node specified!\n";
    return {};
  }

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

  std::unordered_map<std::string, int> nodeOrderMap;

  for (int i = 0; i < executionOrder.size(); i++) {
    nodeOrderMap[executionOrder[i]->getName()] = i;
  }

  #pragma omp parallel
  {
    #pragma omp single
    {
      for (int i = 0; i < executionOrder.size(); i++) {
        auto node = executionOrder[i];

        if (!node->getInputDependencies().empty())
        {
          #pragma omp task depend(in: executionOrder[nodeOrderMap.at(node->getInputDependencies()[0])]) depend(out: executionOrder[i]) shared(resultsMap,cg,executionOrder)
          processNodeHelper(node, cg, resultsMap, 1, executionOrder, nodeOrderMap);

          continue;
        }

        #pragma omp task depend(out: executionOrder[i]) shared(resultsMap,cg)
        processNode(node, cg, resultsMap);
      }
    }
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

void dumpSelection(std::ostream& os, FunctionSet& functions) {
  for (auto& fn : functions) {
    os << fn << "\n";
  }
}

}
