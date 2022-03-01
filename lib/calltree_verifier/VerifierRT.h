//
// Created by sebastian on 15.10.21.
//

#ifndef CAPI_TEST_RT_H
#define CAPI_TEST_RT_H

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

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

    void collectSharedLibs();

    void loadSymTables();

public:
    FunctionNameCache(std::string execFile);

    std::string resolve(const void* addr);
};

class CallTreeLogger {
    std::ofstream out;
    FunctionNameCache& cache;
public:
    CallTreeLogger(std::string filename, FunctionNameCache& cache);

    void onFunctionEnter(void* fn, void* caller, int callDepth);

    void onFunctionExit(void* fn, void* caller, int callDepth);
};

#endif //CAPI_TEST_RT_H
