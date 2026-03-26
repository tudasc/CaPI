//
// Created by sebastian on 02.04.25.
//

#ifndef CAPI_INCLUSIVEMETRICANALYSIS_H
#define CAPI_INCLUSIVEMETRICANALYSIS_H

#include "capi/selection/TraversalHelper.h"
#include "capi/selection/SCC.h"

#include "metadata/TransientMD.h"
#include "cage/generator/NumInstructionsMD.h"
#include "metadata/NumStatementsMD.h"

#include "capi/support/Logging.h"

#include <vector>
#include <queue>

using namespace metacg;

namespace capi {

template<typename MetricTraits>
class InclusiveMetricAnalysis {
 public:
    InclusiveMetricAnalysis() {
    }
      bool run(TraversalHelper& helper);
};

template<typename MetricTraits>
static long getSCCCount(const SCCNode& sccNode) {
    long acc = 0;
    for (const auto& node : sccNode.nodes) {
        acc = MetricTraits::accumulate(acc, MetricTraits::getCount(node));
    }
    return acc;
}

struct Interval {
    int l;
    int r;
};

static void mergeIntervals(std::vector<Interval>& intervals) {
    if (intervals.empty()) return;

    std::sort(intervals.begin(), intervals.end(),
              [](const Interval& a, const Interval& b) { return a.l < b.l; });

    std::vector<Interval> merged;
    merged.push_back(intervals[0]);

    for (size_t i = 1; i < intervals.size(); ++i) {
        auto& last = merged.back();
        if (intervals[i].l <= last.r + 1) {
            last.r = std::max(last.r, intervals[i].r);
        } else {
            merged.push_back(intervals[i]);
        }
    }

    intervals.swap(merged);
}

template<typename MetricTraits>
static long computeInclusiveCount(const SCCNode* node, SCCAnalysisResults& sccResults, TraversalHelper& helper, const std::unordered_map<const SCCNode*, long>& cache) {
    std::vector<const SCCNode*> workQueue;
    workQueue.push_back(node);
    std::unordered_set<const SCCNode*> visitedNodes;
    unsigned long inclusiveCount = 0;

    while (!workQueue.empty()) {
        auto wnode = workQueue.back();
        workQueue.pop_back();
        if (visitedNodes.find(wnode) == visitedNodes.end()) {
            visitedNodes.insert(wnode);

//            if (auto it = cache.find(wnode); it != cache.end()) {
//                inclusiveCount += it->second;
//                continue;
//            }

            long localCount = getSCCCount<MetricTraits>(*wnode);
            if (localCount < 0) {
                logError() << "Encountered invalid count\n";
                return -1;
            }
            inclusiveCount += localCount;

            for (const auto& childNode : sccResults.findAllCallees(wnode, helper)) {
                workQueue.push_back(childNode);
            }
        }
    }
    return inclusiveCount;
}
#define USE_OPTIMIZED 1

#if USE_OPTIMIZED
    template<typename MetricTraits>
    bool InclusiveMetricAnalysis<MetricTraits>::run(TraversalHelper& helper) {

        SCCAnalysisResults sccResults = computeSCCs(helper, true);

        // Build adjacency
        std::unordered_map<const SCCNode*, std::vector<const SCCNode*>> outEdges;
        std::unordered_map<const SCCNode*, int> indegree;

        for (auto& scc : sccResults.sccs) {
            const SCCNode* node = &scc;

            for (auto* callee : sccResults.findAllCallees(node, helper)) {
                outEdges[node].push_back(callee);
                indegree[callee]++;
            }

            if (!indegree.count(node)) indegree[node] = 0;
        }

        logInfo() << "Running topo sort...\n";

        // Topological sort (Kahn)
        std::queue<const SCCNode*> q;
        for (auto& [node, deg] : indegree) {
            if (deg == 0) q.push(node);
        }

        std::vector<const SCCNode*> topo;
        while (!q.empty()) {
            auto* v = q.front();
            q.pop();
            topo.push_back(v);

            for (auto* w : outEdges[v]) {
                if (--indegree[w] == 0) {
                    q.push(w);
                }
            }
        }

        logInfo() << "Assigning indices...\n";


        // Assign topo index
        std::unordered_map<const SCCNode*, int> topoIndex;
        for (size_t i = 0; i < topo.size(); ++i) {
            topoIndex[topo[i]] = i;
        }

        logInfo() << "Precomputing local SCC counts...\n";

        // Precompute local SCC counts
        std::vector<long> localCount(topo.size());
        for (size_t i = 0; i < topo.size(); ++i) {
            localCount[i] = getSCCCount<MetricTraits>(*topo[i]);
        }

        logInfo() << "Computing prefix sums...\n";


        // Prefix sums
        std::vector<long> prefix(topo.size());
        prefix[0] = localCount[0];
        for (size_t i = 1; i < topo.size(); ++i) {
            prefix[i] = prefix[i - 1] + localCount[i];
        }

        auto intervalSum = [&](int l, int r) {
            if (l == 0) return prefix[r];
            return prefix[r] - prefix[l - 1];
        };

        logInfo() << "Merging intervals...\n";

        // Interval storage
        std::unordered_map<const SCCNode*, std::vector<Interval>> intervals;

        // Process nodes in reverse topological order
        for (auto it = topo.rbegin(); it != topo.rend(); ++it) {
            const SCCNode* node = *it;

            std::vector<Interval> ivals;
            int idx = topoIndex[node];

            ivals.push_back({idx, idx});

            for (auto* child : outEdges[node]) {
                auto& childIntervals = intervals[child];
                ivals.insert(ivals.end(), childIntervals.begin(), childIntervals.end());
            }

            mergeIntervals(ivals);

            intervals[node] = std::move(ivals);
        }

        logInfo() << "Computing inclusive counts for " << topo.size() << " SCCs...\n";

        // Compute inclusive counts
        std::unordered_map<const SCCNode*, long> cache;
        size_t numProcessed{0};
        for (auto* node : topo) {

            long sum = 0;
            for (auto& iv : intervals[node]) {
                sum += intervalSum(iv.l, iv.r);
            }
            cache[node] = sum;
            numProcessed++;
            if (numProcessed % (1 + topo.size() / 10) == 0) {
//                logInfo() << ((100 * numProcessed + 1)/ topo.size()) << "% done...\n";
            }
        }

        // Write results back to original nodes
        for (auto& [sccNode, isc] : cache) {
            for (auto& cnode : sccNode->nodes) {
                auto* node = helper.cg.getNode(cnode->getId());
                assert(node && "Node should not be null here");
                node->getOrCreate<typename MetricTraits::MDType>().value = isc;
            }
        }

        return true;
    }
#else
template<typename MetricTraits>
bool InclusiveMetricAnalysis<MetricTraits>::run(TraversalHelper& helper) {
    std::unordered_map<const SCCNode*, long> cache;

    std::queue<const SCCNode*> workQueue;
    std::unordered_set<const SCCNode*> visitedNodes;

    // TODO: Avoid re-running this analysis
    SCCAnalysisResults sccResults = computeSCCs(helper, true);

    for (auto& leaf :  sccResults.findLeaves(helper)) {
        workQueue.push(leaf);
    }

    int numProcessed = 0;
    while (!workQueue.empty()) {
        auto* node = workQueue.front();
        workQueue.pop();
        visitedNodes.insert(node);
        long isc = computeInclusiveCount<MetricTraits>(node, sccResults, helper, cache);
        if (isc < 0) {
            return false;
        }
        cache[node] = isc;
        for (auto* caller : sccResults.findAllCallers(node, helper)) {
            if (visitedNodes.find(caller) == visitedNodes.end()) {
                workQueue.push(caller);
            }
        }
        numProcessed++;
        if (numProcessed % 100 == 0) {
           logInfo() << "Processed " << numProcessed << " nodes...\n";
        }
    }

    //
//    std::vector<std::pair<const SCCNode*, long>> sortedRes;
//    for (auto& [sccNode, isc] : cache) {
//        sortedRes.emplace_back(sccNode, isc);
//    }
//    std::sort(sortedRes.begin(), sortedRes.end(), [](auto& a, auto& b) {return a.second > b.second;});
//    for (auto& [sccNode, isc] : sortedRes) {
//        std::cout << "Inclusive count for " << sccNode->getName() << ": " << isc << "\n";
//    }

    for (auto& [sccNode, isc] : cache) {
        for (auto& cnode : sccNode->nodes) {
            auto* node = helper.cg.getNode(cnode->getId());
            assert(node && "Node should not be null here");
            node->getOrCreate<typename MetricTraits::MDType>().value = isc;
        }
    }
    return true;
}

#endif

struct StatementCountTraits {
    static constexpr const char* key = "isc";

    using MDType = TransientMD<long, StatementCountTraits>;

    static long getCount(const CgNode* node) {
        if (!node->getHasBody()) {
            return 0;
        }
        if (!node->has<NumStatementsMD>()) {
            logError() << "Need NumStatementsMD to compute inclusive statement count\n";
            return -1;
        }
        const auto numStatementsMD = node->get<NumStatementsMD>();
        return numStatementsMD->getNumberOfStatements();
    }

    static long accumulate(long acc, long val) {
        return acc + val;
    }
};

using ISCMD = TransientMD<long, StatementCountTraits>;


struct InstructionCountTraits {
    static constexpr const char* key = "iic";

    using MDType = TransientMD<long, InstructionCountTraits>;

    static long getCount(const CgNode* node) {
        if (!node->getHasBody()) {
            return 0;
        }
        if (!node->has<cage::NumInstructionsMD>()) {
            logWarn() << "Node " << node->getFunctionName() << " does not provide NumInstructionsMD - assuming 0 instructions.\n";
            return 0;
        }
        const auto numInstructionsMd = node->get<cage::NumInstructionsMD>();
        return numInstructionsMd->getNumberOfInstructions();
    }

    static long accumulate(long acc, long val) {
        return acc + val;
    }
};

// TODO: Avoid code duplication
struct InstructionCountSCCTraits {
    static constexpr const char* key = "iicscc";

    using MDType = TransientMD<long, InstructionCountSCCTraits>;

    static long getCount(const CgNode* node) {
        if (!node->getHasBody()) {
            return 0;
        }
        if (!node->has<cage::NumInstructionsMD>()) {
            logWarn() << "Node " << node->getFunctionName() << " does not provide NumInstructionsMD - assuming 0 instructions.\n";
            return 0;
        }
        const auto numInstructionsMd = node->get<cage::NumInstructionsMD>();
        return numInstructionsMd->getNumberOfInstructions();
    }

    static long accumulate(long acc, long val) {
        return std::max(acc, val);
    }
};

using IICMD = TransientMD<long, InstructionCountTraits>;
using IICSCCMD = TransientMD<long, InstructionCountSCCTraits>;

}

#endif  // CAPI_INCLUSIVEMETRICANALYSIS_H
