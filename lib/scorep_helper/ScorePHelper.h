//
// Created by sebastian on 15.10.21.
//

#ifndef SCOREPHELPER_H
#define SCOREPHELPER_H

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

extern "C" {
    struct scorep_compiler_hash_node;
    typedef uint32_t SCOREP_LineNo;

    scorep_compiler_hash_node* scorep_compiler_hash_put(uint64_t key, const char* region_name_mangled, const char* region_name_demangled, const char* file_name, SCOREP_LineNo line_no_begin);
};

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

struct RTInitializer {
    RTInitializer();
};

class FunctionNameCache {
    std::string execFile;
    std::unordered_map<const void*, std::string> nameCache;

    std::map<uintptr_t, MappedSymTable> addrToSymTable;

    std::vector<std::pair<std::string, MappedSymTable>> symTables;
    std::vector<std::pair<std::string, int>> objFiles;

    void loadSymTables();

    void registerSymbolsInScoreP();

public:
    FunctionNameCache(std::string execFile);

    std::string resolve(const void* addr);
};

#endif //SCOREPHELPER_H
