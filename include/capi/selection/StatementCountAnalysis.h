//
// Created by sebastian on 02.04.25.
//

#ifndef CAPI_STATEMENTCOUNTANALYSIS_H
#define CAPI_STATEMENTCOUNTANALYSIS_H

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
class StatementCountAnalysis {
 public:
      bool run(TraversalHelper& helper);
};

template<typename MetricTraits>
static long computeInclusiveStatementCount(const CgNode* node, TraversalHelper& helper, const std::unordered_map<size_t, long>& cache) {
    std::vector<const CgNode*> workQueue;
    workQueue.push_back(node);
    std::unordered_set<const CgNode*> visitedNodes;
    unsigned long inclusiveStmtCount = 0;

    while (!workQueue.empty()) {
        auto wnode = workQueue.back();
        workQueue.pop_back();
        if (visitedNodes.find(wnode) == visitedNodes.end()) {
            visitedNodes.insert(wnode);

            if (auto it = cache.find(wnode->getId()); it != cache.end()) {
                inclusiveStmtCount += it->second;
                continue;
            }

            long perNodeStmtCount = MetricTraits::getCount(wnode);
            if (perNodeStmtCount < 0) {
                logError() << "Encountered invalid count\n";
                return -1;
            }
            inclusiveStmtCount += perNodeStmtCount;

            for (const auto& childNode : helper.get(wnode).findAllCallees()) {
                workQueue.push_back(childNode);
            }
        }
    }
    return inclusiveStmtCount;
}

template<typename MetricTraits>
bool StatementCountAnalysis<MetricTraits>::run(TraversalHelper& helper) {
    std::unordered_map<size_t, long> cache;

    std::queue<const CgNode*> workQueue;
    std::unordered_set<const CgNode*> visitedNodes;

    // TODO: Avoid re-running this analysis
    SCCAnalysisResults sccResults = computeSCCs(helper, true);

    for (auto& leaf :  sccResults.findLeaves(helper)) {
        for (auto&& v : leaf->nodes)
            workQueue.push(v);
    }

    while (!workQueue.empty()) {
        auto* node = workQueue.front();
        workQueue.pop();
        visitedNodes.insert(node);
        long isc = computeInclusiveStatementCount<MetricTraits>(node, helper, cache);
        if (isc < 0) {
            return false;
        }
        cache[node->getId()] = isc;
        for (auto* caller : helper.get(node).findAllCallers()) {
            if (visitedNodes.find(caller) == visitedNodes.end()) {
                workQueue.push(caller);
            }
        }
    }

    for (auto& [id, isc] : cache) {
        auto* node = helper.cg.getNode(id);
        assert(node && "Node should not be null here");
        node->getOrCreate<typename MetricTraits::MDType>().value = isc;
    }
    return true;
}

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
            logError() << "Node " << node->getFunctionName() << " does not provide NumInstructionsMD - assuming 0 instructions.\n";
            return 0;
        }
        const auto numInstructionsMd = node->get<cage::NumInstructionsMD>();
        return numInstructionsMd->getNumberOfInstructions();
    }
};

using IICMD = TransientMD<long, InstructionCountTraits>;

}

#endif  // CAPI_STATEMENTCOUNTANALYSIS_H
