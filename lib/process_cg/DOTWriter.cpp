//
// Created by sebastian on 28.10.21.
//

#include "DOTWriter.h"

#include <ostream>

static inline std::string getNodeId(CGNode& node) {
    return "n_" + std::to_string(reinterpret_cast<uintptr_t>(&node));
}

bool writeDOT(const CallGraph& cg, std::ostream& out) {
    out << "digraph {\n";
    for (auto& node : cg.getNodes()) {
        out << getNodeId(*node) << " [label=\"" << node->getName() << "\"]\n";
    }

    for (auto& node : cg.getNodes()) {
        for (auto& callee : node->getCallees()) {
            out << getNodeId(*node) << " -> " << getNodeId(*callee) << "\n";
        }
    }
    out << "}\n";
    return true;
}
