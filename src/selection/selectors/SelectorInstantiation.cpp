//
// Created by sebastian on 15.03.22.
//

#include "BasicSelectors.h"
#include "CallPathSelector.h"
#include "CommonCallerSelectorSCC.h"
#include "SelectorRegistry.h"
#include "SetOperations.h"
#include "TalpMetricSelector.h"

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
RegisterSelector registerFlopSelector("flops", createMetricSelector<FlopMetric>);

// MemOpSelector
RegisterSelector registerMemOpSelector("memops", createMetricSelector<MemOpMetric>);

// LoopDepthSelector
RegisterSelector registerLoopDepthSelector("loop_depth", createMetricSelector<LoopDepthMetric>);

// CoarseSelector
RegisterSelector coarseSelector("coarse", createSimpleSelector<CoarseSelector>);

// MinCallDepthSelector
RegisterSelector minCallDepthSelector("min_call_depth", createMinCallDepthSelector);

RegisterSelector caSelectorAll("common_caller",
                  createCommmonCallerSelectorSCC<CommonCallerSelectorSCC::ALL>);
RegisterSelector caSelectorPartiallyDistinct("common_caller_partial", createCommmonCallerSelectorSCC<CommonCallerSelectorSCC::PARTIALLY_DISTINCT>);
RegisterSelector caSelectorDistinct("common_caller_distinct",
    createCommmonCallerSelectorSCC<CommonCallerSelectorSCC::DISTINCT>);

RegisterSelector iscSelector("inclusive_statement_count", createMetricSelector<ISCMetric>);

// TALP metrics
RegisterSelector hasTalpMetrics("has_talp_metrics", createSimpleSelector<HasTalpMetricsSelector>);
RegisterSelector cyclesSelector("talp_cycles", createMetricSelector<TalpMetric<TalpMetricKind::CYCLES, long>>);
RegisterSelector instructionSelector("talp_instructions", createMetricSelector<TalpMetric<TalpMetricKind::INSTRUCTIONS, long>>);
RegisterSelector numMeasurements("talp_measurements", createMetricSelector<TalpMetric<TalpMetricKind::NUM_MEASUREMENTS, long>>);
RegisterSelector numMpiCallSelector("talp_mpi_calls", createMetricSelector<TalpMetric<TalpMetricKind::NUM_MPI_CALLS,long>>);
RegisterSelector numOMPParallelsSelector("talp_omp_parallels", createMetricSelector<TalpMetric<TalpMetricKind::NUM_OMP_PARALLELS,long>>);
RegisterSelector numOMPTasksSelector("talp_omp_tasks", createMetricSelector<TalpMetric<TalpMetricKind::NUM_OMP_TASKS,long>>);
RegisterSelector numGPURuntimeCallsSelector("talp_gpu_runtime_calls", createMetricSelector<TalpMetric<TalpMetricKind::NUM_GPU_RUNTIME_CALLS,long>>);
RegisterSelector elapsedTimeSelector("talp_elapsed_time", createMetricSelector<TalpMetric<TalpMetricKind::ELAPSED_TIME, double>>);
RegisterSelector usefulTimeSelector("talp_useful_time", createMetricSelector<TalpMetric<TalpMetricKind::USEFUL_TIME, double>>);
RegisterSelector parEffSelector("talp_parallel_efficiency", createMetricSelector<TalpMetric<TalpMetricKind::PARALLEL_EFFICIENCY, float>>);
RegisterSelector mpiParEffSelector("talp_mpi_parallel_efficiency", createMetricSelector<TalpMetric<TalpMetricKind::MPI_PARALLEL_EFFICIENCY, float>>);
RegisterSelector mpiCommEffSelector("talp_mpi_comm_efficiency", createMetricSelector<TalpMetric<TalpMetricKind::MPI_COMMUNICATION_EFFICIENCY, float>>);
RegisterSelector mpiLoadBalanceSelector("talp_mpi_load_balance", createMetricSelector<TalpMetric<TalpMetricKind::MPI_LOAD_BALANCE, float>>);
RegisterSelector mpiLoadBalanceInSelector("talp_mpi_load_balance_in", createMetricSelector<TalpMetric<TalpMetricKind::MPI_LOAD_BALANCE_IN, float>>);
RegisterSelector mpiLoadBalanceOutSelector("talp_mpi_load_balance_out", createMetricSelector<TalpMetric<TalpMetricKind::MPI_LOAD_BALANCE_OUT, float>>);
RegisterSelector ompParEffSelector("talp_omp_parallel_efficiency", createMetricSelector<TalpMetric<TalpMetricKind::OMP_PARALLEL_EFFICIENCY, float>>);
RegisterSelector ompLoadBalanceSelector("talp_omp_load_balance", createMetricSelector<TalpMetric<TalpMetricKind::OMP_LOAD_BALANCE, float>>);
RegisterSelector ompSchedulingSelector("talp_omp_scheduling_efficiency", createMetricSelector<TalpMetric<TalpMetricKind::OMP_SCHEDULING_EFFICIENCY, float>>);
RegisterSelector ompSerializationSelector("talp_omp_serialization_efficiency", createMetricSelector<TalpMetric<TalpMetricKind::OMP_SERIALIZATION_EFFICIENCY, float>>);
RegisterSelector deviceOffloadSelector("talp_device_offload_efficiency", createMetricSelector<TalpMetric<TalpMetricKind::DEVICE_OFFLOAD_EFFICIENCY, float>>);
RegisterSelector gpuParallelEffSelector("talp_gpu_parallel_efficiency", createMetricSelector<TalpMetric<TalpMetricKind::GPU_PARALLEL_EFFICIENCY, float>>);
RegisterSelector gpuCommEffSelector("talp_gpu_comm_efficiency", createMetricSelector<TalpMetric<TalpMetricKind::GPU_COMMUNICATION_EFFICIENCY, float>>);
RegisterSelector gpuOrchEffSelector("talp_gpu_orch_efficiency", createMetricSelector<TalpMetric<TalpMetricKind::GPU_ORCHESTRATION_EFFICIENCY, float>>);
RegisterSelector averageRegionDurationSelector("talp_avg_region_duration", createMetricSelector<TalpMetric<TalpMetricKind::AVERAGE_REGION_DURATION, float>>);
RegisterSelector averageIPCSelector("talp_avg_ipc", createMetricSelector<TalpMetric<TalpMetricKind::AVERAGE_IPC, float>>);
RegisterSelector averageFREQSelector("talp_avg_freq", createMetricSelector<TalpMetric<TalpMetricKind::AVERAGE_FREQ, float>>);
RegisterSelector dynFilteredSelector("talp_dyn_filtered", createSimpleSelector<TalpDynFilteredSelector>);

using IPCMetric = DerivedMetric<TalpMetric<capi::TalpMetricKind::INSTRUCTIONS, long>, TalpMetric<capi::TalpMetricKind::CYCLES, long>, double, ddivl>;
RegisterSelector ipcSelector("talp_ipc", createMetricSelector<IPCMetric>);


#ifdef CAPI_ENABLE_FLIP
RegisterSelector hasFlipMetric("has_flip_metrics", createSimpleSelector<HasFlipMetricsSelector>);
RegisterSelector flipCyclesSelector("flip_cycles", createMetricSelector<FlipCycles>);
RegisterSelector flipInvocationsSelector("flip_invocations", createMetricSelector<FlipInvocations>);
RegisterSelector flipCyclesPerInvocationSelector("flip_cycles_per_invocation", createMetricSelector<FlipCyclesPerInvoc>);
#endif
}




