//
// Created by sebastian on 14.10.21.
//

#ifndef CAPI_METACGREADER_H
#define CAPI_METACGREADER_H

#include <string>
#include <unordered_map>
#include <vector>

struct FunctionInfo {
  std::string name;

  std::vector<std::string> callees;
  std::vector<std::string> callers;

  bool instrument{false};
  bool containsPointerCall{false};
  std::string fileName;
  bool definedInSystemInclude{false};
  bool isInlined{false};
};

class MetaCGReader {
public:
  using FInfoMap = std::unordered_map<std::string, FunctionInfo>;

  MetaCGReader(std::string filename) : filename(filename) {}

  bool read();

  const FInfoMap &getFunctionInfo() const { return functions; }

private:
  std::string filename;
  FInfoMap functions;

  FInfoMap::mapped_type &getOrInsert(const std::string &key);
};

#endif // CAPI_METACGREADER_H
