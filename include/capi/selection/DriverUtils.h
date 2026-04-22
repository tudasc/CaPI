//
// Created by sebastian on 29.07.25.
//

#ifndef CAPI_DRIVERUTILS_H
#define CAPI_DRIVERUTILS_H

#include "Callgraph.h"

#include "Selector.h"
#include "capi/selection/MeasurementConfig.h"
#include "capi/selection/Preprocessor.h"
#include "capi/selection/SelectionQueryAST.h"
#include "capi/selection/SelectorGraph.h"
#include "capi/symbol_retriever/SymbolRetriever.h"

#include <nlohmann/json.hpp>
#include <expected>

namespace capi {

bool runConsistencyCheck(const metacg::Callgraph& cg);

ASTPtr parseSelectionQuery(const std::string&);

std::string loadFromFile(std::string_view filename);

FunctionSet replaceInlinedFunctions(const SymbolSetList &symSets,
                                    const FunctionSet &functions,
                                    TraversalHelper &helper);

nlohmann::json exportSelectorDoc();

class SelectionRunner {
  using AstCB = std::function<bool(QueryAST&)>;
  using SelectorGraphCB = std::function<bool(SelectorGraph&)>;
  using SelectionResultCB = std::function<bool(InstrumentationAction&, FunctionSet&)>;

 public:
  SelectionRunner(metacg::Callgraph& cg, bool traverseVirtualDtors = false);

  TraversalHelper& getTraversalHelper() {
    return helper;
  }

  std::expected<MeasurementConfig, std::string> runQuery(const std::string& query, bool pathSensitive = false, bool debugMode = false);

  void onASTParsed(AstCB cb) {
    astParsedCBs.push_back(cb);
  }

  void onASTPostProcessed(AstCB cb) {
    astProcessedCBs.push_back(cb);
  }

  void onSelectorGraphBuilt(SelectorGraphCB cb) {
    selectorGraphBuiltCBs.push_back(cb);
  }

  void onSelectorGraphOptimized(SelectorGraphCB cb) {
    selectorGraphOptimizedCBs.push_back(cb);
  }

  void onSelectionResult(SelectionResultCB cb) {
    selectionResultCBs.push_back(cb);
  }

 private:
  metacg::Callgraph& cg;
  TraversalHelper helper;

  std::vector<AstCB> astParsedCBs;
  std::vector<AstCB> astProcessedCBs;
  std::vector<SelectorGraphCB> selectorGraphBuiltCBs;
  std::vector<SelectorGraphCB> selectorGraphOptimizedCBs;
  std::vector<SelectionResultCB> selectionResultCBs;
};

}

#endif  // CAPI_DRIVERUTILS_H
