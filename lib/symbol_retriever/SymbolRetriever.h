//
// Created by sebastian on 22.03.22.
//

#ifndef CAPI_SYMBOLRETRIEVER_H
#define CAPI_SYMBOLRETRIEVER_H

#include <cstdint>
#include <map>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <vector>

namespace capi {

using SymbolTable = std::unordered_map<std::uintptr_t, std::string>;
using SymbolSet = std::unordered_set<std::string>;

using SymTableList = std::vector<std::pair<std::string, SymbolTable>>;
using SymbolSetList = std::vector<std::pair<std::string, SymbolSet>>;

struct MemMapEntry {
  std::string path;
  uintptr_t addrBegin;
  uint64_t offset;
};

struct MappedSymTable {
  SymbolTable table;
  MemMapEntry memMap;
};

using MappedSymTableMap = std::map<uintptr_t, MappedSymTable>;

/**
 * Loads symbols from the running process and maps their addresses into virtual memory.
 * @return
 */
MappedSymTableMap loadMappedSymTables(std::string execFile);

/**
 * Loads symbols from the executable and all shared library dependencies.
 * @param execFile
 * @return
 */
SymTableList loadAllSymTables(std::string execFile);

/**
 * Loads symbols from the executable and all shared library dependencies.
 * Does not save addresses, only symbol names.
 */
SymbolSetList loadSymbolSets(std::string execFile);


inline bool findSymbol(const SymbolSetList& symSets, const std::string& sym) {
  for (auto&& [binary, symSet] : symSets) {
    if (symSet.find(sym) != symSet.end())
      return true;
  }
  return false;
}


std::string getExecPath();

inline uint64_t mapAddrToProc(uint64_t addrInLib, const MappedSymTable& table) {
  return table.memMap.addrBegin + addrInLib - table.memMap.offset;
}

inline std::unordered_set<std::string> getSymbolSet(const SymbolTable& table) {
  std::unordered_set<std::string> symSet;
  for (auto&& [key, val] : table) {
    symSet.insert(val);
  }
  return symSet;
}

}

#endif // CAPI_SYMBOLRETRIEVER_H
