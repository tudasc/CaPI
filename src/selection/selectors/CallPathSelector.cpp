//
// Created by ui72hona on 3/18/26.
//
#include "CallPathSelector.h"

using namespace metacg;

namespace capi {
FunctionSet RootsSelector::apply(const FunctionSetList& input)  {
    if (input.size() != 1) {
        logError() << "Expected exactly one input set, got " << input.size() << " instead.\n";
        return {};
    }

    FunctionSet in = input.front();
    FunctionSet out;
    for (auto& f : in) {
        bool isRoot{true};
        auto visitFn = [&in, &isRoot](const metacg::CgNode &node) {
            if (in.find(&node) != in.end()) {
                isRoot = false;
            }
        };

        std::vector<const CgNode *> alreadyVisited;
        std::unordered_map<const CgNode*, int> minDepth;

        int count = traverseCallGraph(
                *fn, [this](const metacg::CgNode & node, int depth) -> auto {
                    return helper->get(&node).findAllCallees();
                },
                visitFn, false, alreadyVisited, minDepth);

        if (isRoot) {
            out.insert(f);
        }
    }

    }
}
