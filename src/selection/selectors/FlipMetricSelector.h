//
// Created by Peter Arzt on 08.08.25.
//

#ifndef CAPI_FLIPSELECTOR_H
#define CAPI_FLIPSELECTOR_H


#include "BasicSelectors.h"

#include <flip/FLIP_counts.hpp>

namespace capi {

using FlipCounts = flip::runtime::output::FLIPCounts;

template <typename ValT>
class FlipMetric {
public:
  using ValueType = ValT;
  virtual ValueType readMetric(FlipCounts& md) = 0;
};

struct FlipInvocations : FlipMetric<unsigned> {
public:
  virtual unsigned readMetric(FlipCounts& md) override final {
    return md.getInvocationCount();
  }
};

struct FlipCycles : FlipMetric<unsigned> {
public:
  virtual unsigned readMetric(FlipCounts& md) override final {
    return md.getCycleCount();
  }
};

struct FlipCyclesPerInvoc : FlipMetric<float> {
public:
  virtual float readMetric(FlipCounts& md) override final {
    return static_cast<float>(md.getCycleCount()) / static_cast<float>(md.getInvocationCount());
  }
};

template <typename MetricT>
class FlipMetricSelector : public MetricSelector<FlipMetricSelector<MetricT>, FlipCounts, typename MetricT::ValueType> {
  friend class MetricSelector<FlipMetricSelector, FlipCounts, typename MetricT::ValueType>;
  FlipMetricSelector(CmpOp op, Param val) 
  : MetricSelector<FlipMetricSelector, FlipCounts, typename MetricT::ValueType>("FlipMetricsSelector", op, val) {}
public:
  typename MetricT::ValueType readMetric(FlipCounts& md) override {
    MetricT metric;
    return metric.readMetric(md);
  }
};

class HasFlipMetricsSelector: public FilterSelector {
 public:
  explicit HasFlipMetricsSelector() = default;

  bool accept(const metacg::CgNode* fNode) override {
    return fNode->has<FlipCounts>();
  }

  std::string getName() override {
    return "HasFlipMetrics";
  }
};

}

#endif  // CAPI_FLIPSELECTOR_H
