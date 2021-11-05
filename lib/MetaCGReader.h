//
// Created by sebastian on 14.10.21.
//

#ifndef INSTCONTROL_METACGREADER_H
#define INSTCONTROL_METACGREADER_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"

#include <string>

struct FunctionInfo {
    std::string name;
    bool instrument{false};
};

class MetaCGReader {
public:
    using FInfoMap = llvm::StringMap<FunctionInfo>;

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
