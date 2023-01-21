//
// Created by sebastian on 15.03.22.
//

#include "SelectorGraph.h"
#include "../Utils.h"
#include <cassert>
#include <fstream>

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
    if (debugMode) {
      std::string setOutFile = node->getName() + ".txt";
      logInfo() << "Press 'n' to continue, 'p' to print the last function set, or 's' to save it to '" << setOutFile << "'.\n";
      char cmd;
      bool cont{false};
      do {
        std::cin >> cmd;
        if (cmd == 'n') {
          cont = false;
        } else if (cmd == 'p') {
          std::cout << "===== Selection Begin ======\n";
          dumpSelection(std::cout, output);
          std::cout << "===== Selection End ======\n";
          cont = false;
        } else if (cmd == 's') {
          std::ofstream of(setOutFile);
          if (of.good()) {
            dumpSelection(of, output);
          } else {
            std::cerr << "ERROR: Unable to write file.\n";
          }
        } else {
          logInfo() << "Invalid command. Try again.\n";
        }
      } while (cont);
    }
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

void dumpSelection(std::ostream& os, FunctionSet& functions) {
  for (auto& fn : functions) {
    os << fn << "\n";
  }
}

}