//
// Created by sebastian on 28.10.21.
//

#ifndef CAPI_CGTRAITS_H
#define CAPI_CGTRAITS_H

#include "CallGraph.h"
#include "Selector.h"
#include "FunctionFilter.h"

namespace capi {

bool writeDOT(const CallGraph &cg, const FunctionFilter& selection, std::ostream &out);

}

#endif // CAPI_CGTRAITS_H
