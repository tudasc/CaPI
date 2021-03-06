//
// Created by sebastian on 15.03.22.
//

#include "BasicSelectors.h"
#include "CallPathSelector.h"
#include "SetOperations.h"
#include "SelectorRegistry.h"

namespace  {

using namespace capi;

#define CHECK_NUM_ARGS(selectorName, params, expected) \
if (params.size() != expected) { \
      logError() << "##selectorName expects expected arguments, but received " << params.size() << "\n"; \
      return nullptr; \
}

#define CHECK_KIND(param, expected) \
if (param.kind != expected) { \
      logError() << "Expected argument of type expected, but received " << param.kindNames[param.kind] << "\n"; \
      return nullptr; \
}

template<typename SelectorT>
SelectorPtr createSimpleSelector(const std::vector<Param>& params) {
  CHECK_NUM_ARGS(SelectorT, params, 0)
  return std::make_unique<SelectorT>();
}

// NameSelector

SelectorPtr createNameSelector(const std::vector<Param>& params) {
  CHECK_NUM_ARGS(NameSelector, params, 1)
  CHECK_KIND(params[0], Param::STRING)

  auto regexStr = std::get<std::string>(params[0].val);

  return std::make_unique<NameSelector>(regexStr);
}

RegisterSelector registerNameSelector("byName", createNameSelector);

// FilePathSelector

SelectorPtr createFilePathSelector(const std::vector<Param>& params) {
  CHECK_NUM_ARGS(FilePathSelector, params, 1)
  CHECK_KIND(params[0], Param::STRING)

  auto regexStr = std::get<std::string>(params[0].val);

  return std::make_unique<FilePathSelector>(regexStr);
}

template<typename MetricSelectorT>
SelectorPtr createMetricSelector(const std::vector<Param>& params) {
    CHECK_NUM_ARGS(MetricSelector, params, 2)
    CHECK_KIND(params[0], Param::STRING)
    CHECK_KIND(params[1], Param::INT)

    auto opStr = std::get<std::string>(params[0].val);

    auto cmpOp = getCmpOp(opStr);
    if (!cmpOp.has_value()) {
      logError() << "Invalid comparison operator: " << opStr << "\n";
      return nullptr;
    }

    auto intVal = std::get<int>(params[1].val);

    return std::make_unique<MetricSelectorT>(*cmpOp, intVal);
}

RegisterSelector registerFilePathSelector("byPath", createFilePathSelector);

// InlineSelector
RegisterSelector registerInlineSelector("inlineSpecified", createSimpleSelector<InlineSelector>);

// CallPathSelectorUp
RegisterSelector registerCallPathUpSelector("onCallPathTo", createSimpleSelector<CallPathSelector<TraverseDir::TraverseUp>>);

// CallPathSelectorDown
RegisterSelector registerCallPathDownSelector("onCallPathFrom", createSimpleSelector<CallPathSelector<TraverseDir::TraverseDown>>);

// UnionSelector
RegisterSelector registerUnionSelector("join", createSimpleSelector<SetOperationSelector<SetOperation::UNION>>);

// IntersectionSelector
RegisterSelector registerIntersectionSelector("intersect", createSimpleSelector<SetOperationSelector<SetOperation::INTERSECTION>>);

// ComplementSelector
RegisterSelector registerComplementSelector("subtract", createSimpleSelector<SetOperationSelector<SetOperation::COMPLEMENT>>);

// SystemIncludeSelector
RegisterSelector registerSystemHeaderSelector("inSystemHeader", createSimpleSelector<SystemHeaderSelector>);

// UnresolveCallSelector
RegisterSelector registerUnresolvedCallSelector("containsUnresolvedCalls", createSimpleSelector<UnresolvedCallSelector>);

// FlopSelector
RegisterSelector registerFlopSelector("flops", createMetricSelector<FlopSelector>);

// MemOpSelector
RegisterSelector registerMemOpSelector("memOps", createMetricSelector<MemOpSelector>);

// LoopDepthSelector
RegisterSelector registerLoopDepthSelector("loopDepth", createMetricSelector<LoopDepthSelector>);


}




