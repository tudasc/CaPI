//
// Created by Peter Arzt on 03.09.25.
//

#include "FlipMetricSelector.h"

#include "capi/selection/SCC.h"
#include "capi/selection/Selector.h"

using namespace capi;

// helper type containing the value and weight of a (set of) functions that
// are a candiate for instrumentation
struct KnapsackNumbers {
  counter_t weight;
  counter_t value;

  inline double valueDensity() const {
    double value = static_cast<double>(value);
    double weight = static_cast<double>(weight);
    if (value == 0.0) {
      return 0.0;
    } else if (weight == 0.0) {
      return std::numeric_limits<double>::infinity();
    } else {
      return value / weight;
    }
  }

  inline void operator+=(const KnapsackNumbers other) {
    this->weight += other.weight;
    this->value += other.value;
  }
};

// helper type to represent a set of functions that we might want to add to the instrumentation
struct Candidate {
  std::vector<const SCCNode*> members;
  KnapsackNumbers numbers;
};

FunctionSet FlipKnapsackSelector::apply(const FunctionSetList& input) {
  assert(helper);
  if (input.size() != 1) {
    logError() << "Expected exactly one input set, got " << input.size() << " instead.\n";
    return {};
  }

  FunctionSet inputFunctions = input.front();
  const size_t totalNumberOfFunctions = inputFunctions.size();

  // make sure that global counts are available
  const FlipGlobalCounts* overallCounts = helper->cg.get<FlipGlobalCounts>();
  if (overallCounts == nullptr) {
    logError() << "Expected callgraph to have FLIP_counts as global metadata.\n";
    return {};
  }

  // make sure that all functions have FLIP counts
  for (const metacg::CgNode* fct : inputFunctions) {
    if (!fct->has<FlipFunctionCounts>()) {
      logError() << "Function was " << fct->getFunctionName()
                 << " passed to FlipKnapsackSelector, but does not have FLIP counts.\n";
      return {};
    }
  }

  // compute budget
  // net overhead budget = overhead budget after deducting static setup overhead
  const double overallRuntime = overallCounts->getRuntime();
  logInfo() << "Runtime (single rank) as measured by FLIP: " << overallRuntime << "\n";
  logInfo() << "Configured overhead budget: " << _overheadBudget << "\n";
  logInfo() << "Configured setup overhead: " << _setupOverhead << "\n";
  double netOverheadBudget = std::max(1.0, (_overheadBudget * overallRuntime - _setupOverhead) / overallRuntime);

  logInfo() << "Net overhead budget: " << netOverheadBudget << "\n";

  const double runtimeBudget = ((overallCounts->getRuntime() *
                                 static_cast<double>(overallCounts->getInvocationCount()) * (netOverheadBudget - 1.0)));

  logInfo() << "Runtime (all ranks) budget: " << runtimeBudget << "\n";
  const counter_t invocBudget = static_cast<counter_t>(runtimeBudget / _instrumentationCost);
  logInfo() << "Invocation budget: " << invocBudget << "\n";

  // compute graph of strongly-connected components (SCCs)
  SCCAnalysisResults sccResults = computeSCCs(*helper, true);
  // std::cout << "Finished SCC computation" << std::endl;

  // compute ancestor list for each SCC
  std::unordered_map<const SCCNode*, std::vector<const SCCNode*>> sccAncestors =
      sccResults.globalAncestorComputation(*helper);
  // std::cout << "Finished SCC ancestor computation" << std::endl;

  

  // container for strongly-connected subgraphs that are not yet instrumented
  std::unordered_map<const SCCNode*, KnapsackNumbers> sccs;

  // collect all relevant SCCs and compute knapsack weights and values
  for (const metacg::CgNode* node : inputFunctions) {
    const SCCNode* scc = sccResults.getSCC(*node);
    auto it = sccs.find(scc);
    if (!sccs.contains(scc)) {
      KnapsackNumbers values{0, 0};

      for (const metacg::CgNode* sccMemberNode : scc->nodes) {
        const FlipFunctionCounts* callerCounts = sccMemberNode->get<FlipFunctionCounts>();

        if (callerCounts != nullptr && inputFunctions.contains(sccMemberNode)) {
          values.weight += callerCounts->getInvocationCount();
          values.value += callerCounts->getCycleCount();
        }
      }

      sccs.insert({scc, values});
    }
  }
  // std::cout << "Finished computing SCC weights" << std::endl;

  // resulting set of function that we want to instrument
  FunctionSet toBeInstrumented;
  counter_t currentlyInstrumentedInvocs = 0;

  // keep iterating until we run out of functions (or bail out with the break; below)
  unsigned iter = 0;
  while (!sccs.empty()) {
    // std::cout << "Iter " << ++iter << " remaining: " << sccs.size() << "\n";

    Candidate bestCandidate;
    bool foundACandidateThatFits = false;

    for (auto& [scc, values] : sccs) {
      Candidate candidate{{scc}, values.weight, values.value};

      // to prevent gaps in the instrumentation: walk up potential call paths and add all functions to candidate
      for (const SCCNode* ancestor : sccAncestors[scc]) {
        candidate.members.push_back(ancestor);

        const KnapsackNumbers& ancestorValues = sccs[ancestor];
        candidate.numbers += ancestorValues;
      }

      if (
          // does the candidate fit in our budget?
          candidate.numbers.weight + currentlyInstrumentedInvocs <= invocBudget
          // is is better than the previous best?
          && (!foundACandidateThatFits || candidate.numbers.valueDensity() > bestCandidate.numbers.valueDensity())) {
        bestCandidate = candidate;
        foundACandidateThatFits = true;
      }
    }

    if (foundACandidateThatFits && bestCandidate.numbers.valueDensity() > 0.0) {
      // unsigned newlyInstrumented = 0;
      for (const SCCNode* memberSCC : bestCandidate.members) {
        // instrument all functions from the best candidate
        for (const metacg::CgNode* memberNode : memberSCC->nodes) {
          if (inputFunctions.contains(memberNode)) {
            toBeInstrumented.insert(memberNode);
            //  newlyInstrumented++;
          }
        }

        // remove newly instrumented SCCs from the pool
        sccs.erase(memberSCC);
      }
      // std::cout << "Instrumenting: " << newlyInstrumented << std::endl;

      // update already spent budget
      currentlyInstrumentedInvocs += bestCandidate.numbers.weight;
      // std::cout << "currentlyInstrumentedInvocs: " << currentlyInstrumentedInvocs << std::endl;
    } else {
      // there is no longer a candidate that fits in our budget
      break;
    }
  }

  const double expectedOverhead =
      1.0 + (((static_cast<double>(currentlyInstrumentedInvocs) / static_cast<double>(invocBudget)) *
              (static_cast<double>(netOverheadBudget) - 1.0)) +
             (_setupOverhead / overallCounts->getRuntime()));
  logInfo() << "Expected overhead: " << expectedOverhead << ".\n";

  return toBeInstrumented;
}