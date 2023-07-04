//
// Created by sebastian on 28.10.21.
//

#ifndef CAPI_CGTRAITS_H
#define CAPI_CGTRAITS_H

#include "CallGraph.h"
#include "Selector.h"

namespace capi {

bool writeDOT(const CallGraph &cg, const FunctionSet& selection, std::ostream &out);

}

#endif // CAPI_CGTRAITS_H
