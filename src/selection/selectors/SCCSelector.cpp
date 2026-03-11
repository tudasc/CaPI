//
// Created by ui72hona on 3/11/26.
//

#include "SCCSelector.h"

namespace capi {

void SCCSelector::init(capi::TraversalHelper &helper) {
    this->sccResults = computeSCCs(helper, true);
}

FunctionSet SCCSelector::apply(const FunctionSetList &input) {
    FunctionSet out;
    for (auto& functionSet : input) {
        for (auto& f : functionSet) {
            auto* scc = sccResults.getSCC(*f);
            if (!scc) {
                logError() << "No SCC found for function " << f << "\n";
                continue;
            }
            out.insert(scc->nodes.begin(), scc->nodes.end());
        }
    }
    return out;
}


}