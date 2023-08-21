//
// Created by sebastian on 09.08.22.
//

#ifndef CAPI_PREPROCESSOR_H
#define CAPI_PREPROCESSOR_H

#include "InstrumentationHint.h"
#include "SelectionSpecAST.h"

namespace capi {

bool preprocessAST(SpecAST &ast, InstrumentationHints& instHints);

}

#endif // CAPI_PREPROCESSOR_H
