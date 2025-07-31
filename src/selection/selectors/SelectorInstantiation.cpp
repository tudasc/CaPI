//
// Created by sebastian on 15.03.22.
//

#include "BasicSelectors.h"
#include "CallPathSelector.h"
#include "CommonCallerSelectorSCC.h"
#include "SelectorRegistry.h"
#include "SetOperations.h"
#include "TalpMetricSelector.h"

namespace  {

using namespace capi;

#define CHECK_NUM_ARGS(selectorName, params, expected) \
if (params.size() != expected) { \
      logError() << "##selectorName expects expected arguments, but received " << params.size() << "\n"; \
      return nullptr; \
}

#define CHECK_MIN_NUM_ARGS(selectorName, params, expected) \
if (params.size() < expected) { \
      logError() << "##selectorName expects at least " << expected << " arguments, but received " << params.size() << "\n"; \
      return nullptr; \
}

#define CHECK_KIND(param, expected) \
if (param.kind != expected) { \
      logError() << "Expected argument of type " << param.kindNames[expected] << ", but received " << param.kindNames[param.kind] << "\n"; \
      return nullptr; \
}

template<typename SelectorT>
SelectorPtr createSimpleSelector(const std::vector<Param>& params) {
  CHECK_NUM_ARGS(SelectorT, params, 0)
  return std::make_unique<SelectorT>();
}

// NameSelector

SelectorPtr createNameSelector(const std::vector<Param>& params) {
  CHECK_MIN_NUM_ARGS(NameSelector, params, 1)
  CHECK_KIND(params[0], Param::STRING)

  auto regexStr = std::get<std::string>(params[0].val);
  std::vector<std::string> parameterRegexStrings;
  bool isMangled = true;

  if (regexStr[0] != '@') {
    isMangled = false;
      for (int i = 1; i < params.size(); i++) {
        CHECK_KIND(params[i], Param::STRING)
        parameterRegexStrings.push_back(std::get<std::string>(params[i].val));
      }
  }
  else if (params.size() > 1) {
    logError() << "##selectorName expects only one argument when using mangled matching, but received " << params.size() << "\n";
    return nullptr;
  }

  return std::make_unique<NameSelector>(regexStr, parameterRegexStrings, isMangled);
}


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

    auto opStr = std::get<std::string>(params[0].val);

    auto cmpOp = getCmpOp(opStr);
    if (!cmpOp.has_value()) {
      logError() << "Invalid comparison operator: " << opStr << "\n";
      return nullptr;
    }
    auto selectorOrErr = MetricSelectorT::create(*cmpOp, params[1]);
    if (!selectorOrErr) {
      logError() << "Could not instantiate selector: " << selectorOrErr.error() << "\n";
      return {};
    }
    return std::make_unique<MetricSelectorT>(std::move(selectorOrErr.value()));
}


// TODO: Could probably use same function as createMetricSelector
SelectorPtr createMinCallDepthSelector(const std::vector<Param>& params) {
  CHECK_NUM_ARGS(MinCallDepthSelector, params, 2)
  CHECK_KIND(params[0], Param::STRING)
  CHECK_KIND(params[1], Param::INT)

  auto opStr = std::get<std::string>(params[0].val);

  auto cmpOp = getCmpOp(opStr);
  if (!cmpOp.has_value()) {
    logError() << "Invalid comparison operator: " << opStr << "\n";
    return nullptr;
  }

  auto intVal = std::get<int>(params[1].val);
  return std::make_unique<MinCallDepthSelector>(*cmpOp, intVal);
}

template<CommonCallerSelectorSCC::CAHeuristicType T>
SelectorPtr createCommmonCallerSelectorSCC(const std::vector<Param>& params) {
  int maxOrder = INT32_MAX;
  if (!params.empty()) {
    CHECK_NUM_ARGS(CommonCallerSelectorSCC, params, 1)
    CHECK_KIND(params[0], Param::INT)
    maxOrder = std::get<int>(params[0].val);
  }
  return std::make_unique<CommonCallerSelectorSCC>(maxOrder, T);
}

RegisterSelector registerNameSelector("by_name", createNameSelector);

RegisterSelector registerFilePathSelector("by_path", createFilePathSelector);

// InlineSelector
RegisterSelector registerInlineSelector("inline_specified", createSimpleSelector<InlineSelector>);

// CallPathSelectorUp
RegisterSelector registerCallPathUpSelector("on_call_path_to", createSimpleSelector<CallPathSelector<TraverseDir::TraverseUp>>);

// CallPathSelectorDown
RegisterSelector registerCallPathDownSelector("on_call_path_from", createSimpleSelector<CallPathSelector<TraverseDir::TraverseDown>>);

// UnionSelector
RegisterSelector registerUnionSelector("join", createSimpleSelector<SetOperationSelector<SetOperation::UNION>>);

// IntersectionSelector
RegisterSelector registerIntersectionSelector("intersect", createSimpleSelector<SetOperationSelector<SetOperation::INTERSECTION>>);

// ComplementSelector
RegisterSelector registerComplementSelector("subtract", createSimpleSelector<SetOperationSelector<SetOperation::COMPLEMENT>>);

// SystemIncludeSelector
RegisterSelector registerSystemHeaderSelector("in_system_header", createSimpleSelector<SystemHeaderSelector>);

// UnresolveCallSelector
RegisterSelector registerUnresolvedCallSelector("contains_unresolved_calls", createSimpleSelector<UnresolvedCallSelector>);

// FlopSelector
RegisterSelector registerFlopSelector("flops", createMetricSelector<FlopSelector>);

// MemOpSelector
RegisterSelector registerMemOpSelector("memops", createMetricSelector<MemOpSelector>);

// LoopDepthSelector
RegisterSelector registerLoopDepthSelector("loop_depth", createMetricSelector<LoopDepthSelector>);

// CoarseSelector
RegisterSelector coarseSelector("coarse", createSimpleSelector<CoarseSelector>);

// MinCallDepthSelector
RegisterSelector minCallDepthSelector("min_call_depth", createMinCallDepthSelector);

RegisterSelector caSelectorAll("common_caller",
                  createCommmonCallerSelectorSCC<CommonCallerSelectorSCC::ALL>);
RegisterSelector caSelectorPartiallyDistinct("common_caller_partial", createCommmonCallerSelectorSCC<CommonCallerSelectorSCC::PARTIALLY_DISTINCT>);
RegisterSelector caSelectorDistinct("common_caller_distinct",
    createCommmonCallerSelectorSCC<CommonCallerSelectorSCC::DISTINCT>);

RegisterSelector iscSelector("inclusive_statement_count", createMetricSelector<ISCSelector>);

//RegisterSelector hasTalpSelector("") // TODO:

// TALP metrics
// TODO: OMP metrics not added yet
RegisterSelector cyclesSelector("talp_cycles", createMetricSelector<TalpMetricSelector<TalpMetricKind::CYCLES>>);
RegisterSelector instructionSelector("talp_instructions", createMetricSelector<TalpMetricSelector<TalpMetricKind::INSTRUCTIONS>>);
RegisterSelector elapsedTimeSelector("talp_elapsed_time", createMetricSelector<TalpMetricSelector<TalpMetricKind::ELAPSED_TIME>>);
RegisterSelector numMpiCallSelector("talp_mpi_calls", createMetricSelector<TalpMetricSelector<TalpMetricKind::NUM_MPI_CALLS>>);
RegisterSelector parEffSelector("talp_parallel_efficiency", createMetricSelector<TalpMetricSelector<TalpMetricKind::PARALLEL_EFFICIENCY>>);
RegisterSelector mpiParEffSelector("talp_mpi_parallel_efficiency", createMetricSelector<TalpMetricSelector<TalpMetricKind::MPI_PARALLEL_EFFICIENCY>>);
RegisterSelector mpiCommEffSelector("talp_mpi_comm_efficiency", createMetricSelector<TalpMetricSelector<TalpMetricKind::MPI_COMMUNICATION_EFFICIENCY>>);
RegisterSelector mpiLoadBalanceSelector("talp_mpi_load_balance", createMetricSelector<TalpMetricSelector<TalpMetricKind::MPI_LOAD_BALANCE>>);
RegisterSelector mpiLoadBalanceInSelector("talp_mpi_load_balance_in", createMetricSelector<TalpMetricSelector<TalpMetricKind::MPI_LOAD_BALANCE_IN>>);
RegisterSelector mpiLoadBalanceOutSelector("talp_mpi_load_balance_out", createMetricSelector<TalpMetricSelector<TalpMetricKind::MPI_LOAD_BALANCE_OUT>>);

}




