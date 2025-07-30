//
// Created by sebastian on 09.07.25.
//

#ifndef CAPI_MEASUREMENTCONFIGURATION_H
#define CAPI_MEASUREMENTCONFIGURATION_H

#include <vector>
#include <string>
#include <unordered_map>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

namespace capi {

// TODO: Remove duplicate declaration (see InstrumentationHint.h)
using InvocationRange = std::pair<unsigned, unsigned>;
using InvocationRanges = std::vector<InvocationRange>;
using CallPath = std::vector<std::string>;

struct PathEntry {
  CallPath callPath;
  InvocationRanges selectedInvocations;
  std::string measurementLevel;
  std::vector<std::string> flags;
};


using PathEntries = std::vector<PathEntry>;

struct FunctionEntry {
  std::string name;
  PathEntries pathEntries;
};


class MeasurementConfig {
 public:
  void add(const std::string& name, PathEntry entry) {
    auto& functionEntry = selectedFunctions[name];
    functionEntry.pathEntries.push_back(std::move(entry));
  }

  FunctionEntry& get(std::string& name) {
    return selectedFunctions[name];
  }

 private:
  std::unordered_map<std::string, FunctionEntry> selectedFunctions;

  friend void to_json(json&, const MeasurementConfig&);
  friend void from_json(const json&, MeasurementConfig&);
};

// PathEntry
inline void to_json(json& j, const PathEntry& p) {
  j = json{
      {"callPath", p.callPath},
      {"selectedInvocations", p.selectedInvocations},
      {"measurementLevel", p.measurementLevel},
      {"flags", p.flags}
  };
}

inline void from_json(const json& j, PathEntry& p) {
  j.at("callPath").get_to(p.callPath);
  j.at("selectedInvocations").get_to(p.selectedInvocations);
  j.at("measurementLevel").get_to(p.measurementLevel);
  j.at("flags").get_to(p.flags);
}

// FunctionEntry
inline void to_json(json& j, const FunctionEntry& f) {
  j = json{
      {"name", f.name},
      {"pathEntries", f.pathEntries}
  };
}

inline void from_json(const json& j, FunctionEntry& f) {
  j.at("name").get_to(f.name);
  j.at("pathEntries").get_to(f.pathEntries);
}

// MeasurementConfig
inline void to_json(json& j, const MeasurementConfig& config) {
  j = json::object();
  for (const auto& [name, entry] : config.selectedFunctions) {
    j[name] = entry;
  }
}

inline void from_json(const json& j, MeasurementConfig& config) {
  config.selectedFunctions.clear();
  for (const auto& [name, value] : j.items()) {
    FunctionEntry entry = value.get<FunctionEntry>();
    config.selectedFunctions[name] = std::move(entry);
  }
}

}  // namespace capi

#endif  // CAPI_MEASUREMENTCONFIGURATION_H
