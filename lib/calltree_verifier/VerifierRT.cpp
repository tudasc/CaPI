//
// Created by sebastian on 15.10.21.
//

#include "VerifierRT.h"

#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <unistd.h>
#include <cstring>
#include <map>
#include <sstream>


namespace {
    int call_depth{-1};
    std::string exec_path;
    std::unique_ptr<CallTreeLogger> logger;
    std::unique_ptr<FunctionNameCache> name_cache;
    RTInitializer init;
}

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


std::string get_exec_path() {
    RemoveEnvInScope removePreload("LD_PRELOAD");
    char filename[128] = {0};
    auto n = readlink("/proc/self/exe", filename, sizeof(filename) - 1);
    if (n > 0) {
        return filename;
    }
    return "";
}

std::vector<MemMapEntry> read_memory_map() {
    RemoveEnvInScope removePreload("LD_PRELOAD");

    std::vector<MemMapEntry> entries;

    char buffer[256];
    FILE *memory_map = fopen("/proc/self/maps", "r");
    if (!memory_map) {
        std::cout << "Could not load memory map.\n";
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
    // Need to disable LD_PRELOAD, otherwise this library will be loaded in popen call.
    RemoveEnvInScope removePreload("LD_PRELOAD");

    std::string command = "nm --defined-only ";
    if (object_file.find(".so") != std::string::npos) {
        command += "-D ";
    }
    command += object_file;

    char buffer[256] = {0};
    FILE *output = popen(command.c_str(), "r");
    if (!output) {
        std::cerr << "Unable to execute nm to resolve symbol names.\n";
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
//        std::cout << "Found symbol: " << addr << ", " << symName << "\n"; // FIXME Remove
        table[addr] = symName;
    }
    pclose(output);

    return table;

}

std::string resolve_symbol_name(const char *object_file, const void *addr) {
    // Need to disable LD_PRELOAD, otherwise this library will be loaded in popen call.
    RemoveEnvInScope removePreload("LD_PRELOAD");

    char command[256];
    int n = sprintf(command, R"(nm -D %s | grep %lx | awk '{ printf "%%s", $3 }')", object_file,
                    reinterpret_cast<std::uintptr_t>(addr));

    if (n >= sizeof(command) - 1) {
        std::cerr << "Insufficient buffer size for symbol name resolution command.\n";
        return "";
    }
//    std::cout << "Resolve " << addr << ": " << command << "\n";

    char result[128] = {0};
    FILE *output = popen(command, "r");
    if (!output) {
        std::cerr << "Unable to execute nm to resolve symbol name.\n";
        return "";
    }
    if (!fgets(result, sizeof(result), output)) {
        return "";
    }
    pclose(output);

    return result;
}



void print_process_map() {
    RemoveEnvInScope removePreload("LD_PRELOAD");

    char buffer[256];
    FILE *memory_map = fopen("/proc/self/maps", "r");
    if (!memory_map) {
        std::cout << "Could not load process map.\n";
    }

    while (fgets(buffer, sizeof(buffer), memory_map)) {
        std::cout << buffer;
    }
}

RTInitializer::RTInitializer() {
    exec_path = get_exec_path();
    auto execFilename = exec_path.substr(exec_path.find_last_of('/') + 1);
    if (auto val = getenv("VERIFY_ONLY"); val) {
        if (strcmp(val, execFilename.c_str()) != 0) {
            return;
        }
    }
    //    std::cout << "Executable: " << exec_name << "\n";
    print_process_map();
    name_cache = std::make_unique<FunctionNameCache>(exec_path);
    auto logFile = execFilename + ".instro.log";
    logger = std::make_unique<CallTreeLogger>(logFile, *name_cache);
}

FunctionNameCache::FunctionNameCache(std::string execFile) : execFile(execFile) {
    objFiles.emplace_back(execFile, 0);
//    collectSharedLibs();
    loadSymTables();
}

void FunctionNameCache::loadSymTables() {

    // Load symbols from main executable
    auto execSyms = loadSymbolTable(execFile);
    MappedSymTable execTable{std::move(execSyms), {execFile, 0, 0}};
    addrToSymTable[0] = execTable;
//    symTables.emplace_back(execFile, std::move(execTable));


    // Load symbols from shared libs
    auto memMap = read_memory_map();
    for (auto& entry : memMap) {
        auto& filename = entry.path;
        auto table = loadSymbolTable(filename);
        if (table.empty()) {
            std::cout << "Could not load symbols from " << filename << "\n";
            continue;
        }
        std::cout << "Loaded " << table.size() << " symbols from " << filename << "\n";
        //std::cout << "Starting address: " << entry.addrBegin << "\n";
        MappedSymTable mappedTable{std::move(table), entry};
        addrToSymTable[entry.addrBegin] = mappedTable;
        //        symTables.emplace_back(filename, std::move(mappedTable));
    }

//    for (auto& entry : objFiles) {
//        auto& filename = entry.first;
//        auto table = loadSymbolTable(filename);
//        if (table.empty()) {
//            std::cout << "Could not load symbols from " << filename << "\n";
//            continue;
//        }
//        std::cout << "Loaded " << table.size() << " symbols from " << filename << "\n";
//        symTables.emplace_back(filename, std::move(table));
//    }
}

void FunctionNameCache::collectSharedLibs() {
    // Need to disable LD_PRELOAD, otherwise this library will be loaded in popen call.
    RemoveEnvInScope removePreload("LD_PRELOAD");

    char command[256];
    int n = sprintf(command, R"(ldd %s)", execFile.c_str());

    if (n >= sizeof(command) - 1) {
        std::cerr << "Insufficient buffer size for ldd command.\n";
        return;
    }
//    std::cout << "Resolve " << addr << ": " << command << "\n";

    char result[128] = {0};
    FILE *output = popen(command, "r");
    if (!output) {
        std::cerr << "Unable to execute ldd to identify shared libs.\n";
        return;
    }

    std::cout << "Indentifying shared libraries...\n";
    while (fgets(result, sizeof(result), output)) {
        std::string line = result;
        auto arrowPos = line.find("=> ");
        if (arrowPos == std::string::npos) {
            std::cerr << "Line in ldd output ignored: " << line;
            continue;
        }
        auto pathStart = arrowPos + 3;
        std::string libPath = line.substr(pathStart, line.find(' ', pathStart) - pathStart);
        objFiles.emplace_back(libPath, 0);
        std::cout << libPath << "\n";
    }
    std::cout << "Done.\n";

    pclose(output);

}

std::string FunctionNameCache::resolve(const void *addr) {
    if (auto it = nameCache.find(addr); it != nameCache.end()) {
        return it->second;
    }

    auto addrVal = reinterpret_cast<std::uintptr_t>(addr);

//    std::cout << "Looking up address: " << addrVal << "\n";

    auto it = addrToSymTable.lower_bound(addrVal);
    if (it != addrToSymTable.begin()) {
        --it;
        auto& table = it->second.table;
        auto& mapping = it->second.memMap;
//        std::cout << "Start: " << mapping.addrBegin << ", offset: " << mapping.offset <<"\n";
        auto mappedAddr = (addrVal - mapping.addrBegin) + mapping.offset;
//        std::cout << "Looking for mapped adddress " << mappedAddr << " in " << mapping.path << "\n";
        auto entry = table.find(mappedAddr);
        if (entry != table.end()) {
            auto name = entry->second;
            nameCache[addr] = name;
            return name;
        }
    }
//
//    for (auto&& [filename, mappedTable]: symTables) {
//        auto addrVal = reinterpret_cast<std::uintptr_t>(addr);
//        auto it = mappedTable.find(addrVal);
//        if (it != mappedTable.end()) {
//            auto name = it->second;
//            nameCache[addr] = name;
//            return name;
//        }
//    }
//    auto it = objFiles.begin();
//    while (it != objFiles.end()) {
//        auto& filename = it->first;
//        auto name = resolve_symbol_name(filename.c_str(), addr);
//        if (!name.empty()) {
//            nameCache[addr] = name;
//            int count = ++it->second;
//            // Shift lib to the front, if used for many resolutions
//            if (it != objFiles.begin() && count > (it-1)->second) {
//                std::iter_swap(it, it-1);
//            }
//            return name;
//        }
//        it++;
//    }
    std::cerr << "Unable to resolve address\n";
    return "UNKNOWN";
}

CallTreeLogger::CallTreeLogger(std::string filename, FunctionNameCache &cache) : out(std::ofstream(filename)),
                                                                                 cache(cache) {
    std::cout << "Writing call tree log to " << filename << "\n";
}

void CallTreeLogger::onFunctionEnter(void *fn, void *callsite, int callDepth) {
    auto indent = callDepth;
    while (indent-- > 0) {
        out << "  ";
    }
    out << "[ENTER] " << fn << " " << cache.resolve(fn) << " (callsite: " << callsite << " )\n";
}

void CallTreeLogger::onFunctionExit(void *fn, void *callsite, int callDepth) {
    auto indent = callDepth;
    while (indent-- > 0) {
        out << "  ";
    }
    out << "[EXIT] " << fn << " " << cache.resolve(fn) << " (callsite: " << callsite << " )\n";
}

extern "C" {

void __cyg_profile_func_enter(void *fn, void *callsite) {
    call_depth++;
    if (logger) {
        logger->onFunctionEnter(fn, callsite, call_depth);
    }
}

void __cyg_profile_func_exit(void *fn, void *callsite) {
    if (logger) {
        logger->onFunctionExit(fn, callsite, call_depth);
    }
    call_depth--;
}

}
