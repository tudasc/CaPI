//
// Created by sebastian on 28.10.21.
//

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_set>

#include "SelectorRegistry.h"
#include "metadata/OverrideMD.h"

#include "capi/selection/Demangle.h"
#include "capi/selection/DriverUtils.h"
#include "capi/selection/FunctionFilter.h"
#include "capi/selection/MeasurementConfig.h"
#include "capi/support/Logging.h"

#include "capi/selection/QueryParser.h"
#include "capi/selection/SelectorBuilder.h"
#include "capi/selection/SelectorGraph.h"
#include "capi/selection/StatementCountAnalysis.h"
#include "capi/selection/TraversalHelper.h"
#include "capi/selection/metadata/CaPIMD.h"
#include "capi/symbol_retriever/SymbolRetriever.h"
#include "SelectorDocumentation.h"

namespace capi {

nlohmann::json exportSelectorDoc() {
  auto docs = getAllSelectorDocs();

  nlohmann::json result = nlohmann::json::array();

  for (const auto& doc : docs) {
    nlohmann::json j;

    j["name"] = doc.name;
    j["type"] = doc.type;
    j["parameterLabels"] = doc.parameterLabels;
    j["parameterTypes"] = doc.parameterTypes;
    j["examples"] = doc.example;
    j["explanation"] = doc.description;
    j["numInputs"] = doc.numInputs;

    result.push_back(j);
  }

  return result;
}

bool runConsistencyCheck(const metacg::Callgraph& cg) {
  bool success = true;
  for (auto& node : cg.getNodes()) {
    if (auto* overrideMD = node->get<metacg::OverrideMD>(); overrideMD) {
      for (auto id : overrideMD->overrides) {
        auto* baseFunction = cg.getNode(id);
        if (!baseFunction) {
          logError() << "Overridden base function of node " << node->getId() << " (" << node->getFunctionName()
                     << ") does not exist.\n";
          success = false;
          continue;
        }
        if (auto* baseMD = baseFunction->get<metacg::OverrideMD>(); baseMD) {
          if (std::find(baseMD->overriddenBy.begin(), baseMD->overriddenBy.end(), node->getId()) ==
              baseMD->overriddenBy.end()) {
            logError() << "Overridden base function " << baseFunction->getId() << " ("
                       << baseFunction->getFunctionName() << ") does not list overriding function " << node->getId()
                       << " (" << node->getFunctionName() << ") in metadata.\n";
            success = false;
          }
        } else {
          logError() << "Overridden base function " << baseFunction->getId() << " (" << baseFunction->getFunctionName()
                     << ") of node " << node->getId() << " (" << node->getFunctionName()
                     << ") does not define override metadata.\n";
          success = false;
        }
      }

      for (auto id : overrideMD->overriddenBy) {
        auto* overridingFunction = cg.getNode(id);
        if (!overridingFunction) {
          logError() << "Overriding function of node " << node->getId() << " (" << node->getFunctionName()
                     << ") does not exist.\n";
          success = false;
          continue;
        }
        if (auto* overridingMD = overridingFunction->get<metacg::OverrideMD>(); overridingMD) {
          if (std::find(overridingMD->overrides.begin(), overridingMD->overrides.end(), node->getId()) ==
              overridingMD->overrides.end()) {
            logError() << "Overriding function " << overridingFunction->getId() << " (" << node->getFunctionName()
                       << ") does not list base function " << node->getId() << " (" << node->getFunctionName()
                       << ") in metadata.\n";
            success = false;
          }
        } else {
          logError() << "Overriding function " << overridingFunction->getId() << " ("
                     << overridingFunction->getFunctionName() << ") of node " << node->getId() << " ("
                     << node->getFunctionName() << ") does not define override metadata.\n";
          success = false;
        }
      }
    }
  }
  return success;
}

ASTPtr parseSelectionQuery(const std::string& query) {
  auto stripped = stripComments(query);
  QueryParser parser(stripped);
  auto ast = parser.parse();
  return ast;
}

std::string loadFromFile(std::string_view filename) {
  if (filename.empty()) {
    std::cerr << "Given filename is empty!\n";
    return {};
  }

  std::ifstream in(std::string{filename});

  std::string queryStr;

  std::string line;
  while (std::getline(in, line)) {
    queryStr += line + '\n';
  }
  return queryStr;
}

bool isForest(const metacg::Callgraph& cg) {
  for (auto& node : cg.getNodes()) {
    if (cg.getCallers(*node).size() > 1) {
      return false;
    }
  }
  return true;
}

std::optional<std::vector<const metacg::CgNode*>> getCallPath(const metacg::Callgraph& cg, const metacg::CgNode& node) {
  std::vector<const metacg::CgNode*> path;
  auto* n = &node;
  do {
    auto callers = cg.getCallers(*n);
    // No more parents -> return
    if (callers.empty()) {
      return path;
    }
    // Abort if the call path is ambiguous
    if (callers.size() > 1) {
      return std::nullopt;
    }
    n = *callers.begin();
    if (std::find(path.begin(), path.end(), n) != path.end()) {
      // Found cycle
      return std::nullopt;
    }
    path.insert(path.begin(), n);
  } while (true);
  return {};
}

FunctionSet replaceInlinedFunctions(const SymbolSetList& symSets, const FunctionSet& functions,
                                    TraversalHelper& helper) {
  FunctionSet newSet;

  int numAdded = 0;

  std::function<void(const metacg::CgNode&, bool, std::unordered_set<const metacg::CgNode*>)> addValidCallers =
      [&](const metacg::CgNode& node, bool trigger, std::unordered_set<const metacg::CgNode*> visited) {
        visited.insert(&node);
        auto nodeInfo = helper.get(&node);
        for (auto* caller : nodeInfo.getCallers()) {
          if (visited.find(caller) != visited.end()) {
            continue;
          }
          if (findSymbol(symSets, caller->getFunctionName())) {
            if (addToSet(newSet, caller)) {
              if (trigger) {
                assert(caller->has<CaPIMD>());
                caller->get<CaPIMD>()->value.isTrigger = true;
              }
              numAdded++;
            }
          } else {
            addValidCallers(*caller, trigger, visited);
          }
        }
      };

  FunctionSet notFound;

  for (auto& fn : functions) {
    if (findSymbol(symSets, fn->getFunctionName())) {
      newSet.insert(fn);
    } else {
      notFound.insert(fn);
    }
  }
  std::cout << notFound.size()
            << " functions could not be located in the executable, likely due "
               "to inlining.\n";

  int numProcessed = 0;
  int numBetweenOutputs = notFound.size() / 10;
  int nextOutput = numBetweenOutputs;

  for (auto& fn : notFound) {
    if (!fn) {
      std::cerr << "Unable to find function in call graph - skipping.\n";
      continue;
    }
    // Recursively looks for the first available callers and adds them.
    assert(fn->has<CaPIMD>());
    addValidCallers(*fn, fn->get<CaPIMD>()->value.isTrigger, {});
    numProcessed++;

    // Status output
    if (numBetweenOutputs >= 10) {
      if (numProcessed >= nextOutput) {
        logInfo() << (int)((numProcessed / (float)notFound.size()) * 100) << "% of inlined functions processed...\n";
        nextOutput += numBetweenOutputs;
      }
    }
  }

  std::cout << numAdded << " callers of missing functions added.\n";

  return newSet;
}

SelectionRunner::SelectionRunner(metacg::Callgraph& cg, bool traverseVirtualDtors)
    : cg(cg), helper(cg, traverseVirtualDtors) {
  // TODO: Add some kind of analysis management logic for selectors to request results
  StatementCountAnalysis sca;
  sca.run(helper);
  // Ensure that CaPIMD ist present
  bool warned{false};
  for (auto& node : cg.getNodes()) {
    if (!node) {
      continue;
    }
    if (!warned && !node->has<CaPIMD>()) {
      logWarn() << "Found node without CaPI metadata. Make sure to call demangleNames on the input call graph.\n";
      warned = true;
    }
    node->getOrCreate<CaPIMD>();
  }
}

std::expected<MeasurementConfig, std::string> SelectionRunner::runQuery(const std::string& query, bool pathSensitive,
                                                                        bool debugMode) {
  auto ast = parseSelectionQuery(query);
  if (!ast) {
    return std::unexpected("Failed to parse selection query");
  }

  for (auto& cb : astParsedCBs) {
    if (!cb(*ast)) {
      return std::unexpected("Aborted by callback");
    }
  }

  InstrumentationActions actions;
  if (!preprocessAST(*ast, actions)) {
    return std::unexpected("Failed to pre-process query AST");
  }
  for (auto& cb : astProcessedCBs) {
    if (!cb(*ast)) {
      return std::unexpected("Aborted by callback");
    }
  }

  bool instActionsSpecified = !actions.empty();

  auto selectorGraph = buildSelectorGraph(*ast, !instActionsSpecified);
  if (!selectorGraph) {
    return std::unexpected("Could not build selector pipeline");
  }

  // If no actions specified, use full instrumentation of last defined selector instance
  if (!instActionsSpecified) {
    actions.push_back({capi::InstrumentationType::ALWAYS_INSTRUMENT, selectorGraph->getEntryNodes().back()->getName()});
  } else {
    for (auto& action : actions) {
      selectorGraph->addEntryNode(action.selRefName);
    }
  }

  for (auto& cb : selectorGraphBuiltCBs) {
    if (!cb(*selectorGraph)) {
      return std::unexpected("Aborted by callback");
    }
  }

  simplifyGraph(*selectorGraph);

  for (auto& cb : selectorGraphOptimizedCBs) {
    if (!cb(*selectorGraph)) {
      return std::unexpected("Aborted by callback");
    }
  }

  auto result = runSelectorPipeline(*selectorGraph, helper, debugMode);

  MeasurementConfig mc;

  if (pathSensitive && !isForest(cg)) {
    logError() << "Warning: path sensitive selection is only possible if the call graph is a forest.\n";
    pathSensitive = false;
  }

  for (auto& action : actions) {
    auto it = result.find(action.selRefName);
    if (it == result.end()) {
      logError() << "Warning: no selection results for '" << action.selRefName << "'\n";
      continue;
    }
    auto selResult = it->second;

    for (auto& cb : selectionResultCBs) {
      if (!cb(action, selResult)) {
        return std::unexpected("Aborted by callback");
      }
    }

    // Measurement config
    for (auto& f : selResult) {
      CallPath strPath;

      if (pathSensitive) {
        auto path = getCallPath(cg, *f).value_or(std::vector<const metacg::CgNode*>{});
        for (auto* pathNode : path) {
          strPath.push_back(pathNode->getFunctionName());
        }
      }

      auto pathEntry = PathEntry{strPath, action.activeInvocations, {}};

      assert(f->has<CaPIMD>());
      if (action.type == ALWAYS_INSTRUMENT && f->get<CaPIMD>()->value.isTrigger) {
        pathEntry.flags.push_back("scope_trigger");
      }

      // Handle existing entries for that path
      bool shouldAdd = true;
      auto& existingEntries = mc.get(f->getFunctionName());
      for (auto& entry : existingEntries) {
        if (entry.callPath != strPath) {
          continue;
        }
        // Matching path found. For now, print a warning and always override.
        // TODO: Figure out sensible override rules
        logWarn() << "A measurement config entry for function " << f->getFunctionName()
                  << " already exists. Overwriting...\n";
        entry = std::move(pathEntry);
        shouldAdd = false;
      }

      if (shouldAdd) {
        mc.add(f->getFunctionName(), std::move(pathEntry));
      }
    }
  }

  return mc;
}

}  // namespace capi
