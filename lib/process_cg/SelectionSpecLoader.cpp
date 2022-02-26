//
// Created by sebastian on 28.10.21.
//

#include "SelectionSpecLoader.h"

#include <fstream>
#include <map>

#include "Selectors.h"



SelectionSpecLoader::SelectionSpecLoader(const std::string &filename) : filename(filename) {

}


SelectorPtr parseSelector(json& j) {
    auto jType = j["type"];
    if (!jType.is_string()) {
        std::cerr << "Need to specify selector type\n";
        return nullptr;
    }
    auto typeStr = jType.get<std::string>();
    if (typeStr == "all") {
        auto allSelector = selector::all();
        return allSelector;
    } else if (typeStr == "functionName") {
        auto jRegex = j["regex"];
        if (!jRegex.is_string()) {
            std::cerr << "Need to specify function name regex\n";
            return nullptr;
        }
        auto regex = jRegex.get<std::string>();
        auto jInput = j["selector"];
        if (jInput.is_null()) {
            std::cerr << "Selector expects input field\n";
            return nullptr;
        }
        auto inputSelector = parseSelector(jInput);
        if (!inputSelector) {
            return nullptr;
        }
        auto nameSelector = selector::byName(regex, std::move(inputSelector));
        return nameSelector;
    }
    std::cerr << "Unknown selector type\n";
    return nullptr;
}

SelectorPtr SelectionSpecLoader::buildSelector() {
    json j;
    {
        std::ifstream in(filename);
        if (!in.is_open()) {
            std::cerr << "Error: Opening file failed: " << filename << "\n";
            return nullptr;
        }
        if (!json::accept(in)) {
            std::cerr << "Error: Invalid JSON file\n";
            return nullptr;
        }
        in.clear();
        in.seekg(0, std::ios::beg);
        in >> j;
    }

    auto jSelector = j["selector"];
    if (jSelector.is_null()) {
        std::cerr << "Expected top level \"selector\" entry\n";
        return nullptr;
    }
    auto jInput = j["selector"];
    if (jInput.is_null()) {
        std::cerr << "Selector expects input field\n";
        return nullptr;
    }
    auto selector = parseSelector(jInput);
    return selector;
}
