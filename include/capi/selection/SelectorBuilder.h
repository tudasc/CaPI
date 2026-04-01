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
struct SelectorDoc;

void simplifyGraph(SelectorGraph& graph);

SelectorGraphPtr buildSelectorGraph(QueryAST& ast, bool lastDeclIsEntry);
std::optional<SelectorDoc> getSelectorDocumentation(std::string selectorName);
}

#endif // CAPI_SELECTORBUILDER_H
