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

struct FlipInvocations : public SimpleMDMetric<FlipInvocations, FlipFunctionCounts, unsigned> {
 public:
  static constexpr std::string_view Name = "FlipInvocations";
  static unsigned readMDVal(FlipFunctionCounts& md) { return md.getInvocationCount(); }
};

struct FlipCycles : public SimpleMDMetric<FlipCycles, FlipFunctionCounts, unsigned> {
 public:
  static constexpr std::string_view Name = "FlipCycles";
  static unsigned readMDVal(FlipFunctionCounts& md) { return md.getCycleCount(); }
};

struct FlipCyclesPerInvoc : public SimpleMDMetric<FlipCyclesPerInvoc, FlipFunctionCounts, unsigned> {
 public:
  static constexpr std::string_view Name = "FlipCyclesPerInvoc";
  static unsigned readMDVal(FlipFunctionCounts& md) {
    return static_cast<float>(md.getCycleCount()) / static_cast<float>(md.getInvocationCount());
  }
};

class HasFlipMetricsSelector: public FilterSelector {
 public:
  explicit HasFlipMetricsSelector() = default;

  bool accept(const metacg::CgNode* fNode) override { return fNode->has<FlipFunctionCounts>(); }

  std::string getName() override {
    return "HasFlipMetrics";
  }
};

class FlipKnapsackSelector : public Selector {
 public:
  FlipKnapsackSelector(float overheadBudget, float instrumentationCost, float setupOverhead)
      : _overheadBudget(overheadBudget), _instrumentationCost(instrumentationCost), _setupOverhead(setupOverhead) {}

  inline void init(TraversalHelper& helper) override { this->helper = &helper; }

  FunctionSet apply(const FunctionSetList& input) override;

  inline std::string getName() override { return "FlipKnapsackSelector"; }

 private:
  float _overheadBudget;
  float _instrumentationCost;
  float _setupOverhead;
  TraversalHelper* helper;
};
}

#endif  // CAPI_FLIPSELECTOR_H
