//
// Created by sebastian on 11.02.22.
//

#include "FunctionFilter.h"
#include <algorithm>
#include <fstream>
#include <iostream>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

namespace capi {

FunctionFilter::FunctionFilter()
{}

bool FunctionFilter::accepts(const std::string &f) const {
  auto it = instFlags.find(f);
  return it != instFlags.end() && isInstrumented(it->second);
}

int FunctionFilter::getFlags(const std::string &f) const {
  auto it = instFlags.find(f);
  if (it == instFlags.end()) {
    return InstrumentationType::NONE;
  }
  return it->second;
}

void FunctionFilter::addIncludedFunction(const std::string &f, int flags)
{
  auto it = instFlags.find(f);
  if (it == instFlags.end()) {
    instFlags[f] = flags;
  } else {
    it->second |= flags;
  }
}

void FunctionFilter::removeIncludedFunction(const std::string &f)
{

  instFlags.erase(f);
}

bool readScorePFilterFile(FunctionFilter &filter, const std::string &filename)
{
  std::ifstream is(filename);
  if (!is) {
    return false;
  }

  auto accept = [&is, &filter](const std::string &expected) -> bool {
    std::string token;
    is >> token;
    if (token == expected)
      return true;
    std::cerr << "Error reading filter file: expected " << expected << "\n";
    return false;
  };

  if (!accept("SCOREP_REGION_NAMES_BEGIN")) {
    return false;
  }
  if (!accept("EXCLUDE")) {
    return false;
  }
  if (!accept("*")) {
    return false;
  }
  if (!accept("INCLUDE")) {
    return false;
  }
  if (!accept("MANGLED")) {
    return false;
  }

  std::string f;
  while (!is.eof()) {
    is >> f;
    if (f == "SCOREP_REGION_NAMES_END") {
      return true;
    }
    filter.addIncludedFunction(f);
  }
  std::cerr << "Error reading filter file: unexpected end.\n";
  return false;
}

bool writeSimpleFilterFile(FunctionFilter& filter, const std::string& filename) {
  std::ofstream os(filename);
  if (!os) {
    return false;
  }
  for (auto &f : filter) {
    os << f.first << "\n";
  }
  return true;
}

bool writeScorePFilterFile(FunctionFilter &filter,
                           const std::string &filename)
{
  std::ofstream os(filename);
  if (!os) {
    return false;
  }
  os << "SCOREP_REGION_NAMES_BEGIN\n";
  os << "EXCLUDE *\n";
  os << "INCLUDE MANGLED\n";
  for (auto &f : filter) {
    os << f.first << "\n";
  }
  os << "SCOREP_REGION_NAMES_END\n";
  return true;
}

bool writeJSONFilterFile(FunctionFilter &filter, const std::string& filename) {
  std::ofstream os(filename);
  if (!os) {
    return false;
  }
  json j;
  auto& jSelectionMap = j["selection"];
  for (auto &f : filter) {
    json fj;
    fj["flags"] = f.second;
    jSelectionMap[f.first] = fj;
  }
  os << j;
  return true;
}

bool readJSONFilterFile(FunctionFilter &filter, const std::string& filename) {
  std::ifstream in(filename);
  if (!in) {
    return false;
  }
  json j;
  in >> j;
  auto jSelectionMap = j["selection"];
  for (json::iterator it = jSelectionMap.begin(); it != jSelectionMap.end(); ++it) {
    auto& fname = it.key();
    filter.addIncludedFunction(fname, it.value()["flags"].get<int>());
  }
  return true;
}


}