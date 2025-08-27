//
// Created by Peter Arzt on 08.08.25.
//

#ifndef CAPI_FLIPSELECTOR_H
#define CAPI_FLIPSELECTOR_H


#include "BasicSelectors.h"
#include "capi/selection/Selector.h"
#include "capi/support/Logging.h"
#include "CallPathSelector.h"
#include <limits>
#include "selectors/CallPathSelector.h"

#include <Callgraph.h>
#include <CgNode.h>
#include <flip/FLIP_counts.hpp>

namespace capi {

using FlipCounts = flip::runtime::output::FLIPCounts;
using counter_t = flip::runtime::output::counter_t;

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

class FlipKnapsackSelector : public Selector {

public:

  FlipKnapsackSelector(float overheadBudget, float instrumentationCost) : _overheadBudget(overheadBudget), _instrumentationCost(instrumentationCost) {}

  void init(TraversalHelper& helper) override {
    this->helper = &helper;
  }

  FunctionSet apply(const FunctionSetList &input) override {
    assert(helper);

    if (input.size() != 1) {
      logError() << "Expected exactly one input set, got " << input.size() << " instead.\n";
      return {};
    }
    
    const metacg::Callgraph& cg = helper->cg;
    FunctionSet functions = input.front();

    // make sure that global counts are available
    const FlipCounts* overallCounts = cg.get<FlipCounts>();
    if(overallCounts == nullptr) {
      logError() << "Expected callgraph to have FLIP_counts as global metadata.\n";
      return {};
    }

    // make sure that all functions have FLIP counts
    for(const metacg::CgNode* fct : functions) {
      if (!fct->has<FlipCounts>()) {
        logError() << "Function was " << fct->getFunctionName() << " passed to FlipKnapsackSelector, but does not have FLIP counts.\n";
        return {};
      }
    }

    // compute budget
    double cycleBudget = static_cast<double>(overallCounts->getCycleCount()) * static_cast<double>(_overheadBudget);
    counter_t invocBudget = static_cast<counter_t>(cycleBudget / _instrumentationCost);

    FunctionSet toBeInstrumented;
    counter_t currentlyInstrumentedInvocs = 0;

    // helper type to represent a set of functions that we might want to add to the instrumentation
    struct Candidate {
      FunctionSet functions;
      counter_t weight;
      counter_t value;

      double valueDensity() {
        return static_cast<double>(value) / static_cast<double>(weight);
      }
    };
    
    // keep iterating until the budget is exhausted or we instrumented all functions
    while (currentlyInstrumentedInvocs < invocBudget && toBeInstrumented.size() < functions.size()) {
      Candidate bestCandidate;
      double highestValueDensity = std::numeric_limits<double>::min();
      
      for(const metacg::CgNode* fct : functions) {
        const FlipCounts* counts = fct->get<FlipCounts>();

        Candidate candidate{{fct}, counts->getInvocationCount(), counts->getCycleCount()};

        // to prevent gaps in the instrumentation: walk up potential call paths and add all functions to candidate
        traverseCallGraph(
          *fct,
          [this] (const metacg::CgNode& node) -> auto {
            return helper->get(&node).findAllCallers();
          },
          [&toBeInstrumented, &functions, &candidate] (const metacg::CgNode& node) {
            if (functions.contains(&node) && !toBeInstrumented.contains(&node) && !candidate.functions.contains(&node)) {
              candidate.functions.insert(&node);

              const FlipCounts* callerCounts = node.get<FlipCounts>();
              candidate.weight += callerCounts->getInvocationCount();
              candidate.value += callerCounts->getCycleCount();
            }
          }
        );

        // compare with current bestCandidate to determine the candiate with the highest value density
        const double valueDensity = candidate.valueDensity();
        if (valueDensity > highestValueDensity) {
          bestCandidate = candidate;
          highestValueDensity = valueDensity;
        } 
      }

      // instrument all functions from the best candidate
      toBeInstrumented.insert(bestCandidate.functions.begin(), bestCandidate.functions.end());
      currentlyInstrumentedInvocs += bestCandidate.weight;
    }

    return toBeInstrumented;
  }

  std::string getName() override {
    return "FlipKnapsackSelector";
  }

private:
  float _overheadBudget;
  float _instrumentationCost;
  TraversalHelper *helper;
};

}

#endif  // CAPI_FLIPSELECTOR_H
