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


namespace {
    int call_depth{-1};
    std::string exec_name;
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

std::string resolve_symbol_name(const char *exec_name, const void *addr) {
    // Need to disable LD_PRELOAD, otherwise this library will be loaded in popen call.
    RemoveEnvInScope removePreload("LD_PRELOAD");

    char command[256];
    int n = sprintf(command, R"(nm %s | grep %lx | awk '{ printf "%%s", $3 }')", exec_name,
                    reinterpret_cast<std::uintptr_t>(addr));

    if (n >= sizeof(command) - 1) {
        std::cerr << "Insufficient buffer size for symbol name resolution command.\n";
        return "UNKNOWN";
    }
//    std::cout << "Resolve " << addr << ": " << command << "\n";

    char result[128] = {0};
    FILE *output = popen(command, "r");
    if (!output) {
        std::cerr << "Unable to execute nm to resolve symbol name.\n";
        return "UNKNOWN";
    }
    if (!fgets(result, sizeof(result), output)) {
        return "UNKNOWN";
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
    exec_name = get_exec_path();
//    std::cout << "Executable: " << exec_name << "\n";
//    print_process_map();
    name_cache = std::make_unique<FunctionNameCache>(exec_name);
    auto logFile = std::string(exec_name) + ".instro.log";
    logger = std::make_unique<CallTreeLogger>(logFile, *name_cache);
}

FunctionNameCache::FunctionNameCache(std::string execFile) : execFile(execFile) {
}

std::string FunctionNameCache::resolve(const void *addr) {
    if (auto it = nameCache.find(addr); it != nameCache.end()) {
        return it->second;
    }
    auto name = resolve_symbol_name(execFile.c_str(), addr);
    nameCache[addr] = name;
    return name;
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
