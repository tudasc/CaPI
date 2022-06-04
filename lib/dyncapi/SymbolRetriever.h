//
// Created by sebastian on 22.03.22.
//

#ifndef CAPI_SYMBOLRETRIEVER_H
#define CAPI_SYMBOLRETRIEVER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace capi {

using SymbolTable = std::map<std::uintptr_t, std::string>;

struct MemMapEntry {
  std::string path;
  uintptr_t addrBegin;
  uint64_t offset;
};

struct MappedSymTable {
  SymbolTable table;
  MemMapEntry memMap;
};

// TODO: This is copied from ScoreP Symbol Injector - use a commom source base
class SymbolRetriever {
  std::string execFile;

  std::map<uintptr_t, MappedSymTable> addrToSymTable;
  std::vector<std::pair<std::string, MappedSymTable>> symTables;

  bool loadSymTables();

public:
  explicit SymbolRetriever(std::string execFile);

  bool run();

  const std::map<uintptr_t, MappedSymTable>& getMappedSymTables() const {
    return addrToSymTable;
  }

};

std::string getExecPath();

inline uint64_t mapAddrToProc(uint64_t addrInLib, const MappedSymTable& table) {
  return table.memMap.addrBegin + addrInLib - table.memMap.offset;
}

}

#endif // CAPI_SYMBOLRETRIEVER_H
