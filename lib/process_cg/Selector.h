//
// Created by sebastian on 28.10.21.
//

#ifndef INSTCONTROL_SELECTOR_H
#define INSTCONTROL_SELECTOR_H

#include <regex>

#include "MetaCGReader.h"
#include "CallGraph.h"



using FunctionSet = std::vector<std::string>;


class Selector {
public:

    virtual void init(CallGraph& cg) {
    }

    virtual FunctionSet apply() = 0;

};

using SelectorPtr = std::unique_ptr<Selector>;

class EverythingSelector : public Selector {
    FunctionSet allFunctions;
public:
    void init(CallGraph& cg) override {
        for (auto& node : cg.getNodes()) {
            allFunctions.push_back(node->getName());
        }
    }

    FunctionSet apply() override {
        return allFunctions;
    }
};

class FilterSelector : public Selector {
    SelectorPtr input;
public:
    FilterSelector(SelectorPtr in) : input(std::move(in)) {
    }

    void init(CallGraph& cg) override {
        input->init(cg);
    }

    virtual bool accept(const std::string&) = 0;

    FunctionSet apply() override {
        FunctionSet in = input->apply();
//        std::cout << "Input contains " << in.size() << " elements\n";
        in.erase(std::remove_if(in.begin(), in.end(), [this](std::string& fName) -> bool {
            return !accept(fName);
        }), in.end());
        return in;
    }
};

class NameSelector : public FilterSelector{
    std::regex nameRegex;
public:
    NameSelector(SelectorPtr in, std::string regexStr) : FilterSelector(std::move(in)), nameRegex(regexStr) {
    }

    bool accept(const std::string& fName) override;
};

class CallPathSelector : public Selector {
    SelectorPtr input;
    CallGraph* cg;
public:
    CallPathSelector(SelectorPtr input) : input(std::move(input)) {
    }

    void init(CallGraph& cg) override {
        input->init(cg);
        this->cg = &cg;
    }

    FunctionSet apply() override;

};



class SelectorRunner {
    CallGraph& cg;
public:
    SelectorRunner(CallGraph& cg) : cg(cg) {

    }

    FunctionSet run(Selector& selector);
};


#endif //INSTCONTROL_SELECTOR_H
