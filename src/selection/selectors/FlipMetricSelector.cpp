//
// Created by Peter Arzt on 03.09.25.
//

#include "FlipMetricSelector.h"

#include "capi/selection/SCC.h"
#include "capi/selection/Selector.h"
#include "capi/selection/TraversalHelper.h"
#include "capi/support/Logging.h"

#include <metacg/Callgraph.h>
#include <metacg/CgNode.h>
#include <memory>
#include <unordered_set>
#include <vector>

using namespace capi;

// helper type containing the value and weight of a (set of) functions that
// are a candiate for instrumentation
struct KnapsackNumbers {
  counter_t weight;
  counter_t value;

  inline double valueDensity() const {
    if (value == 0) {
      return 0.0;
    } else if (weight == 0) {
      return std::numeric_limits<double>::infinity();
    } else {
      double v = static_cast<double>(value);
      double w = static_cast<double>(weight);
      return v / w;
    }
  }

  inline void operator+=(const KnapsackNumbers other) {
    this->weight += other.weight;
    this->value += other.value;
  }
};

// helper type to represent a set of functions that we might want to add to the instrumentation
struct Candidate {
  const SCCNode* node;
  KnapsackNumbers numbers;
};

FunctionSet FlipKnapsackSelector::apply(const FunctionSetList& input) {
  assert(helper);
  if (input.size() != 1) {
    logError() << "Expected exactly one input set, got " << input.size() << " instead.\n";
    return {};
  }

  metacg::Callgraph& cg = helper->cg;
  FunctionSet inputFunctions = input.front();
  const size_t totalNumberOfFunctions = inputFunctions.size();

  // make sure that global counts are available
  const FlipGlobalCounts* overallCounts = cg.get<FlipGlobalCounts>();
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

  // prune callgraph
  std::vector<const metacg::CgNode*> nodes;
  nodes.reserve(cg.getNodeCount());
  for (const std::unique_ptr<metacg::CgNode>& node : cg.getNodes()) {
    nodes.push_back(node.get());
  }

  // unsigned i = 0;
  for (const metacg::CgNode* node : nodes) {
    // std::cout << ++i << "\n";
    if (!inputFunctions.contains(node)) {
      // auto parents = helper->get(node).findAllCallers();
      // auto children = helper->get(node).findAllCallees();
      auto parents = cg.getCallers(*node);
      auto children = cg.getCallees(*node);

      for (auto parent : parents) {
        for (auto child : children) {
          assert(parent != nullptr);
          assert(child != nullptr);

          if (parent != child && !cg.existsEdge(*parent, *child)) {
            cg.addEdge(*parent, *child);
          }
        }
      }

      cg.erase(node->getId());
    }
  }

  logInfo() << "Pruned call graph down to " << cg.getNodeCount() << " nodes.\n";

  // create new TraversalHelper
  TraversalHelper prunedTravesalHelper(cg, false);

  // compute graph of strongly-connected components (SCCs)
  SCCAnalysisResults sccResults = computeSCCs(prunedTravesalHelper, false);
  // std::cout << "Finished SCC computation" << std::endl;

  std::unordered_set<const SCCNode*> inputSCCs;
  for (const metacg::CgNode* fct : inputFunctions) {
    inputSCCs.insert(sccResults.getSCC(*fct));
  }

  // compute ancestor list for each SCC
  std::unordered_map<const SCCNode*, std::vector<const SCCNode*>> sccAncestors =
      sccResults.globalAncestorComputation(inputSCCs, prunedTravesalHelper);
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
      Candidate candidate{scc, values.weight, values.value};

      // to prevent gaps in the instrumentation: walk up potential call paths and add all functions to candidate
      for (const SCCNode* ancestor : sccAncestors[scc]) {
        if (sccs.contains(ancestor)) {
          const KnapsackNumbers& ancestorValues = sccs[ancestor];
          candidate.numbers += ancestorValues;
        }
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
      // instrument all functions from the best candiate
      for (const metacg::CgNode* memberNode : bestCandidate.node->nodes) {
        if (inputFunctions.contains(memberNode)) {
          toBeInstrumented.insert(memberNode);
        }
      }
      sccs.erase(bestCandidate.node);
      for (const SCCNode* ancestorSCC : sccAncestors[bestCandidate.node]) {
        if (sccs.contains(ancestorSCC)) {
          // instrument all functions from the best candidate ancestors
          for (const metacg::CgNode* memberNode : ancestorSCC->nodes) {
            if (inputFunctions.contains(memberNode)) {
              toBeInstrumented.insert(memberNode);
            }
          }

          // remove newly instrumented SCCs from the pool
          sccs.erase(ancestorSCC);
        }
      }

      // update already spent budget
      currentlyInstrumentedInvocs += bestCandidate.numbers.weight;
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
