//
// Created by sebastian on 27.02.26.
//

#ifndef CAPI_CGUTILS_H
#define CAPI_CGUTILS_H

#include "metacg/Callgraph.h"
#include "metacg/io/MCGReader.h"

#include "Logging.h"

#include <memory>

namespace capi {

inline std::unique_ptr<metacg::Callgraph> loadGraphFromStr(const char* jsonStr) {
  if (!jsonStr) {
    return {};
  }
  auto j = nlohmann::json::parse(jsonStr, nullptr, false);

  if (j.is_discarded()) {
    logError() << "Cannot load call graph: invalid JSON\n";
    return {};
  }

  auto mcgSrc = metacg::io::JsonSource(j);
  auto reader = metacg::io::createReader(mcgSrc);
  if (!reader) {
    logError() << "Unable to read MetaCG file\n";
    return {};
  }
  return reader->read();
}

}

#endif  // CAPI_CGUTILS_H
