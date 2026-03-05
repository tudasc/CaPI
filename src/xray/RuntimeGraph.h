//
// Created by sebastian on 23.02.26.
//

#ifndef CAPI_RUNTIMEGRAPH_H
#define CAPI_RUNTIMEGRAPH_H

#include <memory>

#include "Callgraph.h"
#include "cage/generator/DynamicLinkagePolicy.h"
#include "capi/selection/MeasurementConfig.h"

namespace capi {

std::unique_ptr<metacg::Callgraph> loadGraphFromStr(const char*);

std::unique_ptr<metacg::Callgraph> extractMainGraph();

class RuntimeGraph {

  std::unique_ptr<metacg::Callgraph> fullStaticGraph;
  std::unique_ptr<metacg::Callgraph> patchGraph;
  size_t numIndirectEdges{0};

public:

  RuntimeGraph(std::unique_ptr<metacg::Callgraph> mainGraph) : fullStaticGraph(std::move(mainGraph)), patchGraph(std::make_unique<metacg::Callgraph>()) {
  }

  void mergeLibGraph(const metacg::Callgraph& libGraph) {
    fullStaticGraph->merge(libGraph, cage::DynamicLinkagePolicy{});
  }

  void recordIndirectCall(const std::string& parent, const std::string& child);

  void validateQuery(const MeasurementConfig& cfg);

  metacg::Callgraph* getStaticGraph() {
      if (!fullStaticGraph) {
          return nullptr;
      }
      return fullStaticGraph.get();
  }

  void printStats();

};

}  // namespace capi

#endif  // CAPI_RUNTIMEGRAPH_H
