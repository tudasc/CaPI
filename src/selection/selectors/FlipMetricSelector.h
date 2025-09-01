//
// Created by Peter Arzt on 08.08.25.
//

#ifndef CAPI_FLIPSELECTOR_H
#define CAPI_FLIPSELECTOR_H


#include "BasicSelectors.h"
#include "capi/selection/Selector.h"
#include "capi/support/Logging.h"

#include <Callgraph.h>
#include <CgNode.h>

#include <flip/FLIP_counts.hpp>

#include <queue>

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

  void init(TraversalHelper& helper) override {
    this->helper = &helper;
  }

  FunctionSet apply(const FunctionSetList &input) override {
    assert(helper);

    if (input.size() != 1) {
      logError() << "Expected exactly one input set, got " << input.size() << " instead.\n";
      return {};
    }
    
    FunctionSet notInstrumentedFunctions = input.front();
    const size_t totalNumberOfFunctions = notInstrumentedFunctions.size();
    
    // make sure that global counts are available
    const FlipGlobalCounts* overallCounts = helper->cg.get<FlipGlobalCounts>();
    if(overallCounts == nullptr) {
      logError() << "Expected callgraph to have FLIP_counts as global metadata.\n";
      return {};
    }

    // make sure that all functions have FLIP counts
    for(const metacg::CgNode* fct : notInstrumentedFunctions) {
      if (!fct->has<FlipFunctionCounts>()) {
        logError() << "Function was " << fct->getFunctionName() << " passed to FlipKnapsackSelector, but does not have FLIP counts.\n";
        return {};
      }
    }

    // compute budget
    // net overhead budget = overhead budget after deducting static setup overhead
    const double overallRuntime = overallCounts->getRuntime();
    logInfo() << "Runtime (single rank) as measured by FLIP: " << overallRuntime << "\n";
    logInfo() << "Configured overhead budget: " << _overheadBudget << "\n";
    logInfo() << "Configured setup overhead: " << _setupOverhead << "\n";
    double netOverheadBudget = std::max(
      1.0,
      (_overheadBudget * overallRuntime - _setupOverhead) / overallRuntime
    );

    logInfo() << "Net overhead budget: " << netOverheadBudget << "\n";

    const double runtimeBudget = (
      (
        overallCounts->getRuntime()
        * static_cast<double>(overallCounts->getInvocationCount())
        * (netOverheadBudget - 1.0)
      )
    );

    
    logInfo() << "Runtime (all ranks) budget: " << runtimeBudget << "\n";
    const counter_t invocBudget = static_cast<counter_t>(runtimeBudget / _instrumentationCost);
    logInfo() << "Invocation budget: " << invocBudget << "\n";

    FunctionSet toBeInstrumented;
    counter_t currentlyInstrumentedInvocs = 0;

    // helper type to represent a set of functions that we might want to add to the instrumentation
    struct Candidate {
      FunctionSet functions;
      counter_t weight;
      counter_t value;

      inline double valueDensity() const {
        return static_cast<double>(value) / static_cast<double>(weight);
      }

      inline bool operator<(const Candidate& other) const {
          return valueDensity() < other.valueDensity(); 
      }
    };
    
    // keep iterating until we run out of functions (or bail out with the break; below)
    while (!notInstrumentedFunctions.empty()) {
      std::priority_queue<Candidate> candidates;
      
      for(const metacg::CgNode* fct : notInstrumentedFunctions) {
        const FlipFunctionCounts* counts = fct->get<FlipFunctionCounts>();

        Candidate candidate{{fct}, counts->getInvocationCount(), counts->getCycleCount()};

        // to prevent gaps in the instrumentation: walk up potential call paths and add all functions to candidate
        for (const metacg::CgNode* ancestor : helper->get(fct).findAllAncestors()) {
          if (notInstrumentedFunctions.contains(ancestor) && !candidate.functions.contains(ancestor)) {
            candidate.functions.insert(ancestor);

            const FlipFunctionCounts* callerCounts = ancestor->get<FlipFunctionCounts>();
            candidate.weight += callerCounts->getInvocationCount();
            candidate.value += callerCounts->getCycleCount();
          }
        }

        candidates.push(candidate);
      }

      // find best candidate that still fits in the budget
      while (!candidates.empty() && candidates.top().weight + currentlyInstrumentedInvocs > invocBudget) {
        candidates.pop();
      }
      if (!candidates.empty()) {
        const Candidate& best = candidates.top();

        // instrument all functions from the best candidate
        toBeInstrumented.insert(best.functions.begin(), best.functions.end());
        for (const metacg::CgNode* node : best.functions) {
          notInstrumentedFunctions.erase(node);
        }
        currentlyInstrumentedInvocs += best.weight;
      } else {
        // there is no longer a candidate that fits in our budget
        break;
      }
    }

    const double expectedOverhead = 1.0 + (
      (
        (static_cast<double>(currentlyInstrumentedInvocs) / static_cast<double>(invocBudget))
        * (static_cast<double>(netOverheadBudget) - 1.0)
      )
      + (_setupOverhead / overallCounts->getRuntime())
    );


    logInfo() << "Expected overhead: " << expectedOverhead << ".\n";

    return toBeInstrumented;
  }

  std::string getName() override {
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
