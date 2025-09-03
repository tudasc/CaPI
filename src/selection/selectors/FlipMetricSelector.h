//
// Created by Peter Arzt on 08.08.25.
//

#ifndef CAPI_FLIPSELECTOR_H
#define CAPI_FLIPSELECTOR_H


#include "BasicSelectors.h"
#include "capi/selection/Selector.h"
#include "capi/selection/TraversalHelper.h"

#include <Callgraph.h>
#include <CgNode.h>

#include <flip/FLIP_counts.hpp>

namespace capi {

using FlipFunctionCounts = flip::runtime::output::FLIPFunctionCounts;
using FlipGlobalCounts = flip::runtime::output::FLIPGlobalCounts;
using counter_t = flip::runtime::output::counter_t;

template <typename ValT>
class FlipMetric {
public:
  using ValueType = ValT;
  virtual ValueType readMetric(FlipFunctionCounts& md) = 0;
};

struct FlipInvocations : FlipMetric<unsigned> {
public:
  virtual unsigned readMetric(FlipFunctionCounts& md) override final {
    return md.getInvocationCount();
  }
};

struct FlipCycles : FlipMetric<unsigned> {
public:
  virtual unsigned readMetric(FlipFunctionCounts& md) override final {
    return md.getCycleCount();
  }
};

struct FlipCyclesPerInvoc : FlipMetric<float> {
public:
  virtual float readMetric(FlipFunctionCounts& md) override final {
    return static_cast<float>(md.getCycleCount()) / static_cast<float>(md.getInvocationCount());
  }
};

template <typename MetricT>
class FlipMetricSelector : public MetricSelector<FlipMetricSelector<MetricT>, FlipFunctionCounts, typename MetricT::ValueType> {
  friend class MetricSelector<FlipMetricSelector, FlipFunctionCounts, typename MetricT::ValueType>;
  FlipMetricSelector(CmpOp op, Param val) 
  : MetricSelector<FlipMetricSelector, FlipFunctionCounts, typename MetricT::ValueType>("FlipMetricsSelector", op, val) {}
public:
  typename MetricT::ValueType readMetric(FlipFunctionCounts& md) override {
    MetricT metric;
    return metric.readMetric(md);
  }
};

class HasFlipMetricsSelector: public FilterSelector {
 public:
  explicit HasFlipMetricsSelector() = default;

  bool accept(const metacg::CgNode* fNode) override {
    return fNode->has<FlipFunctionCounts>();
  }

  std::string getName() override {
    return "HasFlipMetrics";
  }
};

class FlipKnapsackSelector : public Selector {
public:

  FlipKnapsackSelector(float overheadBudget, float instrumentationCost, float setupOverhead) 
  : _overheadBudget(overheadBudget),
    _instrumentationCost(instrumentationCost),
    _setupOverhead(setupOverhead) {}

  inline void init(TraversalHelper& helper) override {
    this->helper = &helper;
  }

  FunctionSet apply(const FunctionSetList &input) override;

  inline std::string getName() override {
    return "FlipKnapsackSelector";
  }

private:
  float _overheadBudget;
  float _instrumentationCost;
  float _setupOverhead;
  TraversalHelper *helper;
};
}

#endif  // CAPI_FLIPSELECTOR_H
