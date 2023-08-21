//
// Created by sebastian on 14.10.21.
//

#ifndef CAPI_METACGREADER_H
#define CAPI_METACGREADER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

namespace capi {

struct FunctionInfo
{
  std::string name;
  std::string demangledName;

  std::vector<std::string> callees;
  std::vector<std::string> callers;

  std::vector<std::string> overrides;
  std::vector<std::string> overridenBy;

  std::vector<std::string> virtualCalls;

  bool isVirtual{false};
  bool instrument{false};
  bool containsPointerCall{false};
  std::string fileName;
  bool definedInSystemInclude{false};
  bool isInlined{false};

  json metaData;

};

class MetaCGReader
{
public:
  using FInfoMap = std::unordered_map<std::string, FunctionInfo>;

  MetaCGReader(std::string inputFile) : inputFile(inputFile)
  {}

  bool read();

  const FInfoMap &getFunctionInfo() const
  { return functions; }

private:
  std::string inputFile;
  FInfoMap functions;

  FInfoMap::mapped_type &getOrInsert(const std::string &key);
};
}

#endif // CAPI_METACGREADER_H
