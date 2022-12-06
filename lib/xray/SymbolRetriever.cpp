//
// Created by sebastian on 22.03.22.
//

#include "SymbolRetriever.h"
#include "../Utils.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <unistd.h>

namespace capi {

struct RemoveEnvInScope {

  explicit RemoveEnvInScope(const char* varName) : varName(varName) {
    oldVal = getenv(varName);
    if (oldVal)
      setenv(varName, "", true);
  }

  ~RemoveEnvInScope() {
    if (oldVal)
      setenv(varName, oldVal, true);
  }

private:
  const char* varName;
  const char* oldVal;
};


std::string getExecPath() {
  RemoveEnvInScope removePreload("LD_PRELOAD");
  char filename[128] = {0};
  auto n = readlink("/proc/self/exe", filename, sizeof(filename) - 1);
  if (n > 0) {
    return filename;
  }
  return "";
}

std::vector<MemMapEntry> readMemoryMap() {
  RemoveEnvInScope removePreload("LD_PRELOAD");

  std::vector<MemMapEntry> entries;

  char buffer[256];
  FILE *memory_map = fopen("/proc/self/maps", "r");
  if (!memory_map) {
    logInfo() << "Could not load memory map.\n";
  }

  std::string addrRange;
  std::string perms;
  uint64_t offset;
  std::string dev;
  uint64_t inode;
  std::string path;
  while (fgets(buffer, sizeof(buffer), memory_map)) {
    std::istringstream lineStream(buffer);
    lineStream >> addrRange >> perms >> std::hex >> offset >> std::dec >> dev >> inode >> path;
    //std::cout << "Memory Map: " << addrRange << ", " << perms << ", " << offset << ", " << path <<"\n";
    if (strcmp(perms.c_str(), "r-xp") != 0) {
      continue;
    }
    if (path.find(".so") == std::string::npos) {
      continue;
    }
    uintptr_t addrBegin = std::stoul(addrRange.substr(0, addrRange.find('-')), nullptr, 16);
    //std::cout << std::hex << addrBegin << '\n';
    entries.push_back({path, addrBegin, offset});
  }
  return entries;
}



SymbolTable loadSymbolTable(const std::string& object_file) {
  // Need to disable LD_PRELOAD, otherwise this library will be loaded in popen call, if linked dynamically
  RemoveEnvInScope removePreload("LD_PRELOAD");

  std::string command = "nm --defined-only ";
  if (object_file.find(".so") != std::string::npos) {
    command += "-D ";
  }
  command += object_file;

  char buffer[256] = {0};
  FILE *output = popen(command.c_str(), "r");
  if (!output) {
    logError() << "Unable to execute nm to resolve symbol names.\n";
    return {};
  }

  SymbolTable table;

  uintptr_t addr;
  std::string symType;
  std::string symName;

  while (fgets(buffer, sizeof(buffer), output)) {
    std::istringstream line(buffer);
    if (buffer[0] != '0') {
      continue;
    }
    line >> std::hex >> addr;
    line >> symType;
    line >> symName;
    table[addr] = symName;
  }
  pclose(output);

  if (table.empty()) {
    logError() << "Unable to resolve symbol names for binary " << object_file << "\n";
  }

  return table;

}

SymbolRetriever::SymbolRetriever(std::string execFile) : execFile(std::move(execFile)) {
}

bool SymbolRetriever::run() {
  loadSymTables();
  return true;
}

bool SymbolRetriever::loadSymTables() {

  addrToSymTable.clear();

  // Load symbols from main executable
  auto execSyms = loadSymbolTable(execFile);
  MappedSymTable execTable{std::move(execSyms), {execFile, 0, 0}};
  addrToSymTable[0] = execTable;

  // Load symbols from shared libs
  auto memMap = readMemoryMap();
  for (auto &entry: memMap) {
    auto &filename = entry.path;
    auto table = loadSymbolTable(filename);
    if (table.empty()) {
      logError() << "Could not load symbols from " << filename << "\n";
      continue;
    }

    //std::cout << "Loaded " << table.size() << " symbols from " << filename << "\n";
    //std::cout << "Starting address: " << entry.addrBegin << "\n";
    MappedSymTable mappedTable{std::move(table), entry};
    addrToSymTable[entry.addrBegin] = mappedTable;
    //        symTables.emplace_back(filename, std::move(mappedTable));
  }
  return true;
}



}