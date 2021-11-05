//
// Created by sebastian on 14.10.21.
//

#include "MetaCGReader.h"

#include "llvm/Support/raw_ostream.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include <fstream>

MetaCGReader::FInfoMap::mapped_type& MetaCGReader::getOrInsert(const std::string &key) {
    if (functions.find(key) != functions.end()) {
        auto &fi = functions[key];
        return fi;
    } else {
        FunctionInfo fi;
        fi.name = key;
        functions.insert({key, fi});
        auto &rfi = functions[key];
        return rfi;
    }
}

bool MetaCGReader::read() {
    functions.clear();
    json j;
    {
        std::ifstream in(filename);
        if (!in.is_open()) {
            llvm::errs() << "Error: Opening file failed: " << filename << "\n";
            return false;
        }
        if (!json::accept(in)) {
            llvm::errs() << "Error: Invalid JSON file\n";
            return false;
        }
        in.clear();
        in.seekg(0, std::ios::beg);
        in >> j;
    }

    // TODO: Ignoring version information etc. for now

    auto jsonCG = j["_CG"];
    if (jsonCG.is_null()) {
        llvm::errs() << "Error: The call graph entry in the MetaCG file is null.\n";
        return false;
    }

    for (auto it = jsonCG.begin(); it != jsonCG.end(); ++it) {
        auto &fi = getOrInsert(it.key());

//        auto jCallees = it.value()["callees"];
//        if (!jCallees.is_null()) {
//            std::for_each(jCallees.begin(), jCallees.end(), [&fi](auto& jCallee) {
//               fi.callees.push_back(jCallee.value().get<std::string>());
//            });
//        }

        auto jMeta = it.value()["meta"];
        if (!jMeta.is_null()) {
            auto jInstrument = jMeta["instrument"];
            if (jInstrument.is_boolean()) {
                fi.instrument = jInstrument.get<bool>();
            } else {
                llvm::outs() << "Instrumentation not specified for function " << it.key() << ": not instrumenting.\n";
            }
        } else {
            llvm::outs() << "No metadata for function " << it.key() << ": not instrumenting.\n";
        }
    }
    return true;
}