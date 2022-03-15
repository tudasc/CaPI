//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_SELECTORBUILDER_H
#define CAPI_SELECTORBUILDER_H

#include "SelectionSpecAST.h"
#include "SelectorGraph.h"
#include <cassert>
#include <variant>


namespace capi {

SelectorGraphPtr buildSelectorGraph(SpecAST& ast);

}

#endif // CAPI_SELECTORBUILDER_H
