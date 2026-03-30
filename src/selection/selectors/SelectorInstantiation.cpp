//
// Created by sebastian on 15.03.22.
//

#include "BasicSelectors.h"
#include "CallPathSelector.h"
#include "CommonCallerSelectorSCC.h"
#include "SelectorRegistry.h"
#include "SetOperations.h"
#include "TalpMetricSelector.h"
#include "SelectorDocumentation.h"

#ifdef CAPI_ENABLE_FLIP
#include "FlipMetricSelector.h"
#endif

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

template<typename MetricT>
SelectorPtr createMetricSelector(const std::vector<Param>& params) {
  CHECK_NUM_ARGS(MetricSelector, params, 2)
  CHECK_KIND(params[0], Param::STRING)

  auto opStr = std::get<std::string>(params[0].val);

  auto cmpOp = getCmpOp(opStr);
  if (!cmpOp.has_value()) {
    logError() << "Invalid comparison operator: " << opStr << "\n";
    return nullptr;
  }
  auto selectorOrErr = MetricSelector<MetricT>::create(*cmpOp, params[1]);
  if (!selectorOrErr) {
    logError() << "Could not instantiate selector: " << selectorOrErr.error() << "\n";
    return {};
  }
  return std::make_unique<MetricSelector<MetricT>>(std::move(selectorOrErr.value()));
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

RegisterSelector registerNameSelector("by_name", createNameSelector, getSelectorDocs().at("by_name"));

RegisterSelector registerFilePathSelector("by_path", createFilePathSelector, getSelectorDocs().at("by_path"));

// InlineSelector
RegisterSelector registerInlineSelector("inline_specified", createSimpleSelector<InlineSelector>, getSelectorDocs().at("inline_specified"));

// CallPathSelectorUp
RegisterSelector registerCallPathUpSelector("on_call_path_to", createSimpleSelector<CallPathSelector<TraverseDir::TraverseUp>>, getSelectorDocs().at("on_call_path_to"));

// CallPathSelectorDown
RegisterSelector registerCallPathDownSelector("on_call_path_from", createSimpleSelector<CallPathSelector<TraverseDir::TraverseDown>>, getSelectorDocs().at("on_call_path_from"));

// UnionSelector
RegisterSelector registerUnionSelector("join", createSimpleSelector<SetOperationSelector<SetOperation::UNION>>, getSelectorDocs().at("join"));

// IntersectionSelector
RegisterSelector registerIntersectionSelector("intersect", createSimpleSelector<SetOperationSelector<SetOperation::INTERSECTION>>, getSelectorDocs().at("intersect"));

// ComplementSelector
RegisterSelector registerComplementSelector("subtract", createSimpleSelector<SetOperationSelector<SetOperation::COMPLEMENT>>, getSelectorDocs().at("subtract"));

// SystemIncludeSelector
RegisterSelector registerSystemHeaderSelector("in_system_header", createSimpleSelector<SystemHeaderSelector>, getSelectorDocs().at("in_system_header"));

// UnresolveCallSelector
RegisterSelector registerUnresolvedCallSelector("contains_unresolved_calls", createSimpleSelector<UnresolvedCallSelector>, getSelectorDocs().at("contains_unresolved_calls"));

// FlopSelector
RegisterSelector registerFlopSelector("flops", createMetricSelector<FlopMetric>, getSelectorDocs().at("flops"));

// MemOpSelector
RegisterSelector registerMemOpSelector("memops", createMetricSelector<MemOpMetric>, getSelectorDocs().at("memops"));

// LoopDepthSelector
RegisterSelector registerLoopDepthSelector("loop_depth", createMetricSelector<LoopDepthMetric>, getSelectorDocs().at("loop_depth"));

// CoarseSelector
RegisterSelector coarseSelector("coarse", createSimpleSelector<CoarseSelector>, getSelectorDocs().at("coarse"));

// MinCallDepthSelector
RegisterSelector minCallDepthSelector("min_call_depth", createMinCallDepthSelector, getSelectorDocs().at("min_call_depth"));

RegisterSelector caSelectorAll("common_caller",
                  createCommmonCallerSelectorSCC<CommonCallerSelectorSCC::ALL>, selectorDocs["common_caller"]);
RegisterSelector caSelectorPartiallyDistinct("common_caller_partial", createCommmonCallerSelectorSCC<CommonCallerSelectorSCC::PARTIALLY_DISTINCT>, selectorDocs["common_caller_partial"]);
RegisterSelector caSelectorDistinct("common_caller_distinct",
    createCommmonCallerSelectorSCC<CommonCallerSelectorSCC::DISTINCT>, selectorDocs["common_caller_distinct"]);

RegisterSelector iscSelector("inclusive_statement_count", createMetricSelector<ISCMetric>, selectorDocs["inclusive_statement_count"]);

// TALP metrics
// TODO: OMP metrics not added yet
RegisterSelector hasTalpMetrics("has_talp_metrics", createSimpleSelector<HasTalpMetricsSelector>, getSelectorDocs().at("has_talp_metrics"));
RegisterSelector cyclesSelector("talp_cycles", createMetricSelector<TalpMetric<TalpMetricKind::CYCLES, long>>, getSelectorDocs().at("talp_cycles"));
RegisterSelector instructionSelector("talp_instructions", createMetricSelector<TalpMetric<TalpMetricKind::INSTRUCTIONS, long>>, getSelectorDocs().at("talp_instructions"));
RegisterSelector numMeasurements("talp_measurements", createMetricSelector<TalpMetric<TalpMetricKind::NUM_MEASUREMENTS, long>>, getSelectorDocs().at("talp_measurements"));
RegisterSelector elapsedTimeSelector("talp_elapsed_time", createMetricSelector<TalpMetric<TalpMetricKind::ELAPSED_TIME, long>>, getSelectorDocs().at("talp_elapsed_time"));
RegisterSelector numMpiCallSelector("talp_mpi_calls", createMetricSelector<TalpMetric<TalpMetricKind::NUM_MPI_CALLS,long>>, getSelectorDocs().at("talp_mpi_calls"));
RegisterSelector parEffSelector("talp_parallel_efficiency", createMetricSelector<TalpMetric<TalpMetricKind::PARALLEL_EFFICIENCY, float>>, getSelectorDocs().at("talp_parallel_efficiency"));
RegisterSelector mpiParEffSelector("talp_mpi_parallel_efficiency", createMetricSelector<TalpMetric<TalpMetricKind::MPI_PARALLEL_EFFICIENCY, float>>, getSelectorDocs().at("talp_mpi_parallel_efficiency"));
RegisterSelector mpiCommEffSelector("talp_mpi_comm_efficiency", createMetricSelector<TalpMetric<TalpMetricKind::MPI_COMMUNICATION_EFFICIENCY, float>>, getSelectorDocs().at("talp_mpi_comm_efficiency"));
RegisterSelector mpiLoadBalanceSelector("talp_mpi_load_balance", createMetricSelector<TalpMetric<TalpMetricKind::MPI_LOAD_BALANCE, float>>, getSelectorDocs().at("talp_mpi_load_balance"));
RegisterSelector mpiLoadBalanceInSelector("talp_mpi_load_balance_in", createMetricSelector<TalpMetric<TalpMetricKind::MPI_LOAD_BALANCE_IN, float>>, getSelectorDocs().at("talp_mpi_load_balance_in"));
RegisterSelector mpiLoadBalanceOutSelector("talp_mpi_load_balance_out", createMetricSelector<TalpMetric<TalpMetricKind::MPI_LOAD_BALANCE_OUT, float>>, getSelectorDocs().at("talp_mpi_load_balance_out"));
RegisterSelector dynFilteredSelector("talp_dyn_filtered", createSimpleSelector<TalpDynFilteredSelector>, getSelectorDocs().at("talp_dyn_filtered"));

using IPCMetric = DerivedMetric<TalpMetric<capi::TalpMetricKind::INSTRUCTIONS, long>, TalpMetric<capi::TalpMetricKind::CYCLES, long>, double, ddivl>;
RegisterSelector ipcSelector("talp_ipc", createMetricSelector<IPCMetric>, getSelectorDocs().at("talp_avg_ipc")); // TODO: check if talp_ipc corresponds to talp_avg_ipc


#ifdef CAPI_ENABLE_FLIP
RegisterSelector hasFlipMetric("has_flip_metrics", createSimpleSelector<HasFlipMetricsSelector>, getSelectorDocs().at("has_flip_metrics"));
RegisterSelector flipCyclesSelector("flip_cycles", createMetricSelector<FlipCycles>, getSelectorDocs().at("flip_cycles"));
RegisterSelector flipInvocationsSelector("flip_invocations", createMetricSelector<FlipInvocations>, getSelectorDocs().at("flip_invocations"));
RegisterSelector flipCyclesPerInvocationSelector("flip_cycles_per_invocation", createMetricSelector<FlipCyclesPerInvoc>, getSelectorDocs().at("flip_cycles_per_invocation"));
#endif
}




