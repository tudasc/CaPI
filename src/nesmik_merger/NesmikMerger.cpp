//
// Created by ui72hona on 3/2/26.
//

#include "capi/nesmik_merger/NesmikMerger.h"

#include "LoggerUtil.h"
#include "io/MCGReader.h"
#include "io/VersionFourMCGReader.h"
#include "io/VersionFourMCGWriter.h"
#include "io/VersionTwoMCGReader.h"
#include "io/VersionTwoMCGWriter.h"

#include "capi/selection/metadata/TalpMD.h"
#include "capi/support/Logging.h"

#include <fstream>
#include <iostream>

using namespace metacg;

namespace capi {

static std::unique_ptr<TalpMD> getTalpMD(const std::string& id, const nlohmann::json& talp_json, bool filtered) {

    // get the TalpMetrics using the from_json method in the plugin
    if (!talp_json["Application"].contains(id)) {
        logWarn() << "No metrics found for function " << id << "\n";
        return nullptr;
    }
    TalpMetrics metrics = talp_json["Application"][id];

//    logInfo() << "Metrics found: " << talp_json["Application"][id] << "\n";

    auto metadata = std::make_unique<TalpMD>();
    metadata->setMetrics(metrics);
    metadata->setDynamicallyFiltered(filtered);

    return metadata;
}

std::unique_ptr<Callgraph> buildDynamicGraphAndAttachMetrics(Callgraph* staticGraph, const nlohmann::json& talp_json,
                                                             const nlohmann::json& nesmik_json, const std::unordered_set<std::string>& dynamic_filter_list) {

    // Create new dynamic CG
    auto dynamic_cg = std::make_unique<metacg::Callgraph>();

    auto copyNodeFromStaticCG = [&staticGraph, &dynamic_cg](const std::string& name) -> metacg::CgNode& {
        // We expect there to be at most one node with this name
        CgNode* static_node = nullptr;
        if (staticGraph) {
            static_node = staticGraph->getFirstNode(name);
        }

        auto& new_node = dynamic_cg->insert(name);;
        if (static_node) {
            new_node.setHasBody(static_node->getHasBody());
            new_node.setOrigin(static_node->getOrigin());
            // TODO: Transfer existing metadata as well
        }
        return new_node;
    };

    auto getNodeForRegion = [&dynamic_cg, &copyNodeFromStaticCG](auto& region_names) -> metacg::CgNode*{
        if (region_names.empty()) {
            return nullptr;
        }

        metacg::CgNode* current_parent = nullptr;
        // First, make sure we find the correct root node (or create one)
        for (auto& node_id : dynamic_cg->getNodes(region_names[0])) {
            // We found our node if the name matches and there is no caller
            if (dynamic_cg->getCallers(node_id).empty()) {
                current_parent = dynamic_cg->getNode(node_id);
                break;
            }
        }
        if (!current_parent) {
            current_parent = &copyNodeFromStaticCG(region_names[0]);
        }

        // Get or create the node for each callee level
        int idx = 1;
        while (idx < region_names.size()) {
            metacg::CgNode* matching_node = nullptr;
            for (auto callee : dynamic_cg->getCallees(*current_parent)) {
                if (callee->getFunctionName() == region_names[idx].template get<std::string>()) {
                    matching_node = callee;
                    break;
                }
            }
            if (!matching_node) {
                matching_node = &copyNodeFromStaticCG(region_names[idx]);
                dynamic_cg->addEdge(*current_parent, *matching_node);
            }
            current_parent = matching_node;
            idx++;
        }
        return current_parent;
    };

    logInfo() << "Found " << nesmik_json["regions"].size() << " regions recorded at runtime. Now creating annotated call graph...\n";

    // Create node for every nesmik region
    for (const auto& [key, list_of_regions] : nesmik_json["regions"].items()) {
        auto* node = getNodeForRegion(list_of_regions);
        if (!node) {
            logError() << "Failed to create node for function " << key << "\n";
            continue;
        }
        // Attach TALP metadata
        auto md = getTalpMD(key, talp_json, dynamic_filter_list.count(node->getFunctionName()) > 0);
        if (!md) {
            logError() << "Missing data in TALP file: No metrics for function " << node->getFunctionName() << "(" << key << ")\n";
        } else {
            node->addMetaData(std::move(md));
        }
    }
    return dynamic_cg;
}
}