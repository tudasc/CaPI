//
// Created by sebastian on 14.10.21.
//

#include "MetaCGReader.h"

//#include "llvm/Support/raw_ostream.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include <fstream>
#include <iostream>

namespace capi {

MetaCGReader::FInfoMap::mapped_type &
MetaCGReader::getOrInsert(const std::string &key) {
  if (functions.find(key) != functions.end()) {
    auto &fi = functions[key];
    return fi;
  } else {
    FunctionInfo fi;
    fi.name = key;
    functions.insert({key, fi});
    auto &rfi = functions[key];
    return rfi;
  }
}

bool MetaCGReader::read() {
  functions.clear();
  json j;
  {
    std::ifstream in(filename);
    if (!in.is_open()) {
      std::cerr << "Error: Opening file failed: " << filename << "\n";
      return false;
    }
    if (!json::accept(in)) {
      std::cerr << "Error: Invalid JSON file\n";
      return false;
    }
    in.clear();
    in.seekg(0, std::ios::beg);
    in >> j;
  }

  // TODO: Ignoring version information etc. for now

  auto jsonCG = j["_CG"];
  if (jsonCG.is_null()) {
    std::cerr << "Error: The call graph entry in the MetaCG file is null.\n";
    return false;
  }

  for (auto it = jsonCG.begin(); it != jsonCG.end(); ++it) {
    auto &fi = getOrInsert(it.key());

    auto jCallees = it.value()["callees"];
    if (!jCallees.is_null()) {
      std::for_each(jCallees.begin(), jCallees.end(), [&fi](auto &jCallee) {
        fi.callees.push_back(jCallee.template get<std::string>());
      });
    }

    auto jCallers = it.value()["callers"];
    if (!jCallers.is_null()) {
      std::for_each(jCallers.begin(), jCallers.end(), [&fi](auto &jCaller) {
        fi.callers.push_back(jCaller.template get<std::string>());
      });
    }

    auto jMeta = it.value()["meta"];
    if (!jMeta.is_null()) {
      auto jPointerCall = jMeta["containsPointerCall"];
      if (jPointerCall.is_boolean()) {
        fi.containsPointerCall = jPointerCall.template get<bool>();
      }
      auto jFileProps = jMeta["fileProperties"];
      if (!jFileProps.is_null()) {
        auto jOrigin = jFileProps["origin"];
        auto jSystemInclude = jFileProps["systemInclude"];
        if (jOrigin.is_string()) {
          fi.fileName = jOrigin.get<std::string>();
        }
        if (jSystemInclude.is_boolean()) {
          fi.definedInSystemInclude = jSystemInclude.get<bool>();
        }
      }
      auto jInlined = jMeta["isInlined"];
      if (jInlined.is_boolean()) {
        fi.isInlined = jInlined.get<bool>();
      }
    }
  }
  return true;
}
}