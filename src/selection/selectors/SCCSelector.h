//
// Created by ui72hona on 3/11/26.
//

#ifndef CAPI_SCCSELECTOR_H
#define CAPI_SCCSELECTOR_H

#include "capi/selection/Selector.h"
#include "capi/selection/SCC.h"

namespace capi {

class SCCSelector : public Selector {
    SCCAnalysisResults sccResults;
public:
    SCCSelector() = default;

    void init(TraversalHelper& helper) override;

    FunctionSet apply(const FunctionSetList &input) override;

    std::string getName() override {
        return "SCCSelector";
    }

};

}

#endif //CAPI_SCCSELECTOR_H
