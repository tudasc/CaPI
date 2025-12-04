//
// Created by Peter Arzt on 08.08.25.
//

#ifndef CAPI_FLIPSELECTOR_H
#define CAPI_FLIPSELECTOR_H


#include "BasicSelectors.h"

#include <flip/FLIP_counts.hpp>

namespace capi {

using FlipCounts = flip::runtime::output::FLIPCounts;

struct FlipInvocations : public SimpleMDMetric<FlipInvocations, FlipCounts, unsigned> {
public:
 static constexpr std::string_view Name = "FlipInvocations";
  static unsigned readMDVal(FlipCounts& md) {
    return md.getInvocationCount();
  }
};

struct FlipCycles : public SimpleMDMetric<FlipCycles, FlipCounts, unsigned> {
 public:
  static constexpr std::string_view Name = "FlipCycles";
  static unsigned readMDVal(FlipCounts& md) {
    return md.getCycleCount();
  }
};

struct FlipCyclesPerInvoc : public SimpleMDMetric<FlipCyclesPerInvoc, FlipCounts, unsigned> {
 public:
  static constexpr std::string_view Name = "FlipCyclesPerInvoc";
  static unsigned readMDVal(FlipCounts& md) {
    return static_cast<float>(md.getCycleCount()) / static_cast<float>(md.getInvocationCount());
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
