//
// Created by sebastian on 08.07.25.
//

#ifndef CAPI_TALPSELECTOR_H
#define CAPI_TALPSELECTOR_H

#include "BasicSelectors.h"
#include "metadata/TalpMD.h"

namespace capi {

enum class TalpMetricKind {
  NUM_CPUS,
  CYCLES,
  INSTRUCTIONS,
  NUM_MEASUREMENTS,
  NUM_MPI_CALLS,
  NUM_OMP_PARALLELS,
  NUM_OMP_TASKS,
  ELAPSED_TIME,
  PARALLEL_EFFICIENCY,
  MPI_PARALLEL_EFFICIENCY,
  MPI_COMMUNICATION_EFFICIENCY,
  MPI_LOAD_BALANCE,
  MPI_LOAD_BALANCE_IN,
  MPI_LOAD_BALANCE_OUT,
  OMP_PARALLEL_EFFICIENCY,
  OMP_LOAD_BALANCE,
  OMP_SCHEDULING_EFFICIENCY,
  OMP_SERIALIZATION_EFFICIENCY
};

/**
 * A little ugly, but very efficient.
 * @tparam MetricT
 * @param metrics
 * @return
 */
template <TalpMetricKind MetricT>
double lookupMetric(const TalpMetrics& metrics) {
  switch (MetricT) {
    case TalpMetricKind::NUM_CPUS:
      return metrics.num_cpus;
    case TalpMetricKind::CYCLES:
      return metrics.cycles;
    case TalpMetricKind::INSTRUCTIONS:
      return metrics.instructions;
    case TalpMetricKind::NUM_MEASUREMENTS:
      return metrics.num_measurements;
    case TalpMetricKind::NUM_MPI_CALLS:
      return metrics.num_mpi_calls;
    case TalpMetricKind::NUM_OMP_PARALLELS:
      return metrics.num_omp_parallels;
    case TalpMetricKind::NUM_OMP_TASKS:
      return metrics.num_omp_tasks;
    case TalpMetricKind::ELAPSED_TIME:
      return metrics.elapsed_time;
    case TalpMetricKind::PARALLEL_EFFICIENCY:
      return metrics.parallel_efficiency;
    case TalpMetricKind::MPI_PARALLEL_EFFICIENCY:
      return metrics.mpi_parallel_efficiency;
    case TalpMetricKind::MPI_COMMUNICATION_EFFICIENCY:
      return metrics.mpi_communication_efficiency;
    case TalpMetricKind::MPI_LOAD_BALANCE:
      return metrics.mpi_load_balance;
    case TalpMetricKind::MPI_LOAD_BALANCE_IN:
      return metrics.mpi_load_balance_in;
    case TalpMetricKind::MPI_LOAD_BALANCE_OUT:
      return metrics.mpi_load_balance_out;
    case TalpMetricKind::OMP_PARALLEL_EFFICIENCY:
      return metrics.omp_parallel_efficiency;
    case TalpMetricKind::OMP_LOAD_BALANCE:
      return metrics.omp_load_balance;
    case TalpMetricKind::OMP_SCHEDULING_EFFICIENCY:
      return metrics.omp_scheduling_efficiency;
    case TalpMetricKind::OMP_SERIALIZATION_EFFICIENCY:
      return metrics.omp_serialization_efficiency;
    default:
      break;
  }
  logError() << "Unhandled metric type: " << (int)MetricT << "\n";
  abort();
}

template <TalpMetricKind MetricT, typename ValT>
class TalpMetric : public SimpleMDMetric<TalpMetric<MetricT, ValT>, TalpMD, ValT> {

 public:
  static constexpr std::string_view Name = "TALPMetric";
  static ValT readMDVal(TalpMD& md) {
    std::vector<std::string> path; // TODO: Refactor
    auto metrics = md.findMetrics(path);
    if (!metrics) {
      logError() << "unable to find metrics for path\n";
      return 0;
    }
    return lookupMetric<MetricT>(*metrics);
  }
};

//template <typename T1, typename T2, typename ValT, typename Op>
//class DerivedMetricSelector : public MetricSelector<TalpMetricSelector<MetricT, ValT>, TalpMD, ValT> {
//  friend class MetricSelector<TalpMetricSelector<MetricT, ValT>, TalpMD, ValT>;
//  TalpMetricSelector(CmpOp op, Param val) : MetricSelector<TalpMetricSelector<MetricT, ValT>, TalpMD, ValT>("TalpMetricsSelector", op, val) {}
// public:
//  ValT readMetric(TalpMD& md) override {
//    std::vector<std::string> path; // TODO: Refactor
//    auto metrics = md.findMetrics(path);
//    if (!metrics) {
//      logError() << "unable to find metrics for path\n";
//      return 0;
//    }
//    return lookupMetric<MetricT>(*metrics);
//  }
//};

class HasTalpMetricsSelector: public FilterSelector {
 public:
  explicit HasTalpMetricsSelector() = default;

  bool accept(const metacg::CgNode* fNode) override {
    return fNode->has<TalpMD>();
  }

  std::string getName() override {
    return "HasTalpMetrics";
  }
};

class TalpDynFilteredSelector: public FilterSelector {
 public:
  explicit TalpDynFilteredSelector() = default;

  bool accept(const metacg::CgNode* fNode) override {
    if (auto* md = fNode->get<TalpMD>(); md) {
      return md->wasDynamicallyFiltered();
    }
    logError() << "TALP metrics not available for function " << fNode->getFunctionName() << ".\n";
    return false;
  }

  std::string getName() override {
    return "TalpDynFiltered";
  }
};

}

#endif  // CAPI_TALPSELECTOR_H
