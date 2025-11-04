//
// Created by sebastian on 09.08.22.
//

#ifndef CAPI_PREPROCESSOR_H
#define CAPI_PREPROCESSOR_H

#include "capi/selection/InstrumentationAction.h"
#include "capi/selection/SelectionQueryAST.h"

namespace capi {

bool preprocessAST(QueryAST&ast, InstrumentationActions& instHints);

}

#endif // CAPI_PREPROCESSOR_H
