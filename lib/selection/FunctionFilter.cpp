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
  return std::find(includedFunctionsMangled.begin(), includedFunctionsMangled.end(), f) != includedFunctionsMangled.end();
}

void FunctionFilter::addIncludedFunction(const std::string &f)
{
  includedFunctionsMangled.push_back(f);
}

void FunctionFilter::removeIncludedFunction(const std::string &f)
{
  includedFunctionsMangled.erase(std::remove(includedFunctionsMangled.begin(),
                                             includedFunctionsMangled.end(), f),
                                 includedFunctionsMangled.end());
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
    os << f << "\n";
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
    os << f << "\n";
  }
  os << "SCOREP_REGION_NAMES_END\n";
  return true;
}

bool writeJSONFilterFile(FunctionFilter &filter, const std::vector<std::string>& triggers, const std::string& filename) {
  std::ofstream os(filename);
  if (!os) {
    return false;
  }
  json j;
  auto& fList = j["selection"];
  for (auto &f : filter) {
    json fj;
    fj["name"] = f;
    fj["isTrigger"] = std::find(triggers.begin(), triggers.end(), f) != triggers.end();
    fList.push_back(fj);
  }
  os << j;
  return true;
}

bool readJSONFilterFile(FunctionFilter &filter, std::vector<std::string>& triggers, const std::string& filename) {
  std::ifstream in(filename);
  if (!in) {
    return false;
  }
  json j;
  in >> j;
  auto fList = j["selection"];
  for (auto &fj : fList) {
    filter.addIncludedFunction(fj["name"]);
    if (fj["isTrigger"].get<bool>()) {
      triggers.push_back(fj["name"]);
    }
  }
  return true;
}


}