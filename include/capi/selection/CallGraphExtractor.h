//
// Created by sebastian on 27.02.26.
//

#ifndef CAPI_CALLGRAPHEXTRACTOR_H
#define CAPI_CALLGRAPHEXTRACTOR_H

#include "metacg/Callgraph.h"

#include <filesystem>
#include <memory>

namespace capi {

std::unique_ptr<metacg::Callgraph> extractCallGraph(const std::filesystem::path&);

std::unique_ptr<metacg::Callgraph> extractAndAssembleFullCallGraph(const std::filesystem::path&);

}

#endif  // CAPI_CALLGRAPHEXTRACTOR_H
