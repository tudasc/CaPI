//
// Created by sebastian on 14.10.21.
//

#ifndef INSTCONTROL_METACGREADER_H
#define INSTCONTROL_METACGREADER_H

//#include "llvm/ADT/DenseMap.h"
//#include "llvm/ADT/StringMap.h"

#include <unordered_map>
#include <string>
#include <vector>

struct FunctionInfo {
    std::string name;

    std::vector<std::string> callees;
    std::vector<std::string> callers;

    bool instrument{false};
};

class MetaCGReader {
public:
    using FInfoMap = std::unordered_map<std::string, FunctionInfo>;

    MetaCGReader(std::string filename) : filename(filename)
    {}

    bool read();

    const FInfoMap& getFunctionInfo() const {
        return functions;
    }

private:
    std::string filename;
    FInfoMap functions;

    FInfoMap::mapped_type& getOrInsert(const std::string &key);

};


#endif //INSTCONTROL_METACGREADER_H
