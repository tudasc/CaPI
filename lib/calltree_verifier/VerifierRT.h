//
// Created by sebastian on 15.10.21.
//

#ifndef INSTCONTROL_TEST_RT_H
#define INSTCONTROL_TEST_RT_H

#include <fstream>
#include <string>
#include <unordered_map>

struct RTInitializer {
    RTInitializer();
};

class FunctionNameCache {
    std::string execFile;
    std::unordered_map<const void*, std::string> nameCache;
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

#endif //INSTCONTROL_TEST_RT_H
