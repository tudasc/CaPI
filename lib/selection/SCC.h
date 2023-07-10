//
// Created by sebastian on 10.07.23.
//

#ifndef CAPI_SCC_H
#define CAPI_SCC_H

#include "CallGraph.h"

namespace capi {

std::vector<std::vector<const CGNode*>> computeSCCs(const capi::CallGraph&);

}

#endif // CAPI_SCC_H
