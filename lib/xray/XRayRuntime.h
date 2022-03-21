//
// Created by sebastian on 21.03.22.
//

#ifndef CAPI_XRAYINTERFACE_H
#define CAPI_XRAYINTERFACE_H

#include <string>
#include <map>
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
  SymbolRetriever(std::string execFile);

  bool run();

  const std::map<uintptr_t, MappedSymTable>& getMappedSymTables() const {
    return addrToSymTable;
  }

};


void initXRay();

}

#endif // CAPI_XRAYINTERFACE_H
