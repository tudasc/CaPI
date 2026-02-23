//
// Created by sebastian on 23.02.26.
//

#include "RuntimeGraph.h"

#include "capi/support/Logging.h"
#include "capi/selection/DriverUtils.h"
#include "capi/selection/Demangle.h"

#include "Callgraph.h"
#include "io/MCGReader.h"

#include <memory>

extern "C" {
extern const char __start_metacg[] __attribute__((weak));
extern const char __stop_metacg[] __attribute__((weak));
}

namespace capi {

std::unique_ptr<metacg::Callgraph> loadGraphFromStr(const char* jsonStr) {
  if (!jsonStr) {
    return {};
  }
  auto j = nlohmann::json::parse(jsonStr, nullptr, false);

  if (j.is_discarded()) {
    logError() << "Cannot load embedded call graph: invalid JSON\n";
    return {};
  }

  auto mcgSrc = metacg::io::JsonSource(j);
  auto reader = metacg::io::createReader(mcgSrc);
  if (!reader) {
    logError() << "Unable to read MetaCG file\n";
    return {};
  }
  return reader->read();
}


std::unique_ptr<metacg::Callgraph> extractMainGraph() {
  if (!__start_metacg) {
    return {};
  }
  return loadGraphFromStr(__start_metacg);
}

void RuntimeGraph::recordIndirectCall(const std::string& parent, const std::string& child) {
    if (!patchGraph->existsAnyEdge(parent, child)) {
        metacg::CgNode& caller = patchGraph->getOrInsertNode(parent);
        metacg::CgNode& callee = patchGraph->getOrInsertNode(child);

        // set hasBody to true so the call-graphs can be fully merged
        caller.setHasBody(true);
        callee.setHasBody(true);

        logInfo() << "Recorded indirect call: " << parent << " -> " << child << "\n";
        patchGraph->addEdge(caller, callee);
    }
}

void RuntimeGraph::validateQuery(const MeasurementConfig& cfg) {
    logInfo() << "Validating static call graph...";

    if (patchGraph->isEmpty()) {
        logInfo() << "Patch graph is empty - nothing to do.\n";
        return;
    }

    // TODO: Do we need to save the original graph?
    fullStaticGraph->merge(*patchGraph, metacg::MergeByName());

    logInfo() << "Query: \n" << cfg.getQuery() << "\n";

    demangleNames(*fullStaticGraph);

    std::cout << "Running consistency check on CG...\n";
    bool consistencyCheckSucceeded = runConsistencyCheck(*fullStaticGraph);
    if (consistencyCheckSucceeded) {
        std::cout << "Success!\n";
    } else {
        std::cout << "Consistency check failed!\n";
        std::cout << "The resulting analysis may be faulty.\n";
    }

    // TODO: Store options like 'traverseVirtualDtors' in config file
    // Create selection runner
    SelectionRunner runner(*fullStaticGraph, false);

    // TODO: Store 'pathSensitive' option
    // Execute the query
    auto resultOrErr = runner.runQuery(cfg.getQuery(), false, false);

    if (!resultOrErr) {
        std::cerr << "Selection query failed with error: " << resultOrErr.error() << "\n";
        return;
    }

    auto patchedMc = *resultOrErr;

    auto numAdded = patchedMc.entries().size() - cfg.entries().size();
    if (numAdded > 0) {
        float relIncrease = ((float) numAdded) / cfg.entries().size();

        logInfo() << "Number of missed functions due to unresolved call edges: " << numAdded << " ("
                  << (relIncrease * 100) << "% of total)\n";
        logInfo() << "The following additional functions would be instrumented using the patched graph: \n";
        for (auto &[key, val]: patchedMc.entries()) {
            auto *foundEntry = cfg.get(key);
            if (!foundEntry) {
                auto *node = fullStaticGraph->getFirstNode(key);
                logInfo() << "  " << key << "\n";
                // TODO: Print ISC?
            }
        }
    } else {
        logInfo() << "There is no change due to indirect calls!\n";
    }

    // TODO: Save patch graph
}


void RuntimeGraph::printStats() {
  if (!fullStaticGraph) {
    logInfo() << "No runtime graph loaded\n";
    return;
  }
  logInfo() << "Runtime graph has " << fullStaticGraph->size() << " nodes\n";
  logInfo() << "Patch graph has " << patchGraph->size() << " nodes\n";
}



}  // namespace capi