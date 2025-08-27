//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_SELECTORBUILDER_H
#define CAPI_SELECTORBUILDER_H

#include "SelectionQueryAST.h"
#include "SelectorGraph.h"
#include <cassert>
#include <variant>


namespace capi {

SelectorGraphPtr buildSelectorGraph(QueryAST& ast, bool lastDeclIsEntry);

}

#endif // CAPI_SELECTORBUILDER_H
