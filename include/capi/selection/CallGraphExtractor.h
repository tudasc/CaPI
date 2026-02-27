//
// Created by sebastian on 27.02.26.
//

#ifndef CAPI_CALLGRAPHEXTRACTOR_H
#define CAPI_CALLGRAPHEXTRACTOR_H

#include "Callgraph.h"

#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

#include <filesystem>
#include <memory>

namespace capi {

std::unique_ptr<metacg::Callgraph> extractCallGraph(const std::filesystem::path&);

std::unique_ptr<metacg::Callgraph> extractAndAssembleFullCallGraph(const std::filesystem::path&);

}

#endif  // CAPI_CALLGRAPHEXTRACTOR_H
