//
// Created by sebastian on 28.10.21.
//

#ifndef CAPI_SELECTOR_H
#define CAPI_SELECTOR_H

#include <regex>
#include <iostream>

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
protected:
    CallGraph* cg;
public:
    FilterSelector(SelectorPtr in) : input(std::move(in)) {
    }

    void init(CallGraph& cg) override {
        input->init(cg);
        this->cg = &cg;
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

enum class SetOperation {
    UNION, INTERSECTION, COMPLEMENT
};

template<SetOperation Op>
class SetOperationSelector: public Selector {
    SelectorPtr inputA;
    SelectorPtr inputB;

public:

    SetOperationSelector(SelectorPtr inA, SelectorPtr inB) : inputA(std::move(inA)), inputB(std::move(inB)) {
    }

    void init(CallGraph& cg) override {
        inputA->init(cg);
        inputB->init(cg);
    }

    FunctionSet apply() override;

};

class IncludeListSelector: public FilterSelector {
    std::vector<std::string> names;
public:
    IncludeListSelector(SelectorPtr in, std::vector<std::string> names) : FilterSelector(std::move(in)), names(names) {
    }

    bool accept(const std::string& fName) override;
};

class ExcludeListSelector: public FilterSelector {
    std::vector<std::string> names;
public:
    ExcludeListSelector(SelectorPtr in, std::vector<std::string> names) : FilterSelector(std::move(in)), names(names) {
    }

    bool accept(const std::string& fName) override;
};

class NameSelector : public FilterSelector{
    std::regex nameRegex;
public:
    NameSelector(SelectorPtr in, std::string regexStr) : FilterSelector(std::move(in)), nameRegex(regexStr) {
    }

    bool accept(const std::string& fName) override;
};

class InlineSelector : public FilterSelector {
public:
    InlineSelector(SelectorPtr in) : FilterSelector(std::move(in)) {
    }

    bool accept(const std::string& fName) override;
};

class FilePathSelector : public FilterSelector{
    std::regex nameRegex;
public:
    FilePathSelector(SelectorPtr in, std::string regexStr) : FilterSelector(std::move(in)), nameRegex(regexStr) {
    }

    bool accept(const std::string& fName) override;
};

class SystemIncludeSelector : public FilterSelector{
    std::regex nameRegex;
public:
    SystemIncludeSelector(SelectorPtr in) : FilterSelector(std::move(in))  {
    }

    bool accept(const std::string& fName) override;
};

class UnresolvedCallSelector : public Selector {
    SelectorPtr input;
    CallGraph* cg;
public:
    explicit UnresolvedCallSelector(SelectorPtr in) : input(std::move(in)) {
    }

    void init(CallGraph& cg) override {
        input->init(cg);
        this->cg = &cg;
    }

    FunctionSet apply() override;
};

enum class TraverseDir {
    TraverseUp, TraverseDown
};

template<TraverseDir dir>
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

/**
 * Traverses the call chain downwards, calling the given visit function on each node.
 * @tparam VisitFn Function that takes a CGNode& argument and returns the next nodes to traverse.
 * @tparam VisitFn Function that takes a CGNode& argument.
 * @param node
 * @param visit
 * @returns The number of visited functions.
 */
template<typename TraverseFn, typename VisitFn>
int traverseCallGraph(CGNode& node, TraverseFn&& selectNextNodes, VisitFn&& visit) {
    std::vector<CGNode*> workingSet;
    std::vector<CGNode*> alreadyVisited;

    workingSet.push_back(&node);

    do {
        auto currentNode = workingSet.back();
        workingSet.pop_back();
//        std::cout << "Visiting caller " << currentNode->getName() << "\n";
        visit(*currentNode);
        alreadyVisited.push_back(currentNode);
        for (auto& nextNode : selectNextNodes(*currentNode)) {
            if (std::find(workingSet.begin(), workingSet.end(), nextNode) == workingSet.end() &&
                std::find(alreadyVisited.begin(), alreadyVisited.end(), nextNode) == alreadyVisited.end()) {
                workingSet.push_back(nextNode);
            }
        }

    } while(!workingSet.empty());

    return alreadyVisited.size();
}



template<TraverseDir Dir>
FunctionSet CallPathSelector<Dir>::apply() {
    static_assert(Dir == TraverseDir::TraverseDown || Dir == TraverseDir::TraverseUp);

    FunctionSet in = input->apply();
    FunctionSet out(in);

    auto visitFn = [&out](CGNode &node) {
        if (std::find(out.begin(), out.end(), node.getName()) == out.end()) {
            out.push_back(node.getName());
        }
    };

    for (auto& fn : in) {
        auto fnNode = cg->get(fn);

        if constexpr(Dir == TraverseDir::TraverseDown) {
            int count = traverseCallGraph(*fnNode, [](CGNode& node) -> auto {return node.getCallees();}, visitFn);
            std::cout << "Functions on callpath from " << fn << ": " << count << "\n";
        } else if constexpr (Dir == TraverseDir::TraverseUp) {
            int count = traverseCallGraph(*fnNode, [](CGNode& node) -> auto {return node.getCallers();}, visitFn);
            std::cout << "Functions on callpath to " << fn << ": " << count << "\n";
        }
    }
    return out;
}

template<SetOperation Op>
FunctionSet SetOperationSelector<Op>::apply() {
    static_assert(Op == SetOperation::UNION || Op == SetOperation::INTERSECTION || Op == SetOperation::COMPLEMENT);

    FunctionSet inA = inputA->apply();
    FunctionSet inB = inputB->apply();

    FunctionSet out;

    if constexpr(Op == SetOperation::UNION) {
        out.insert(out.end(), inA.begin(), inA.end());
        out.insert(out.end(), inB.begin(), inB.end());
    } else if constexpr(Op == SetOperation::INTERSECTION) {
        for (auto& fA : inA) {
            if (std::find(inB.begin(), inB.end(), fA) != inB.end()) {
                out.push_back(fA);
            }
        }
    } else if constexpr(Op == SetOperation::COMPLEMENT) {
        for (auto& fA : inA) {
            if (std::find(inB.begin(), inB.end(), fA) == inB.end()) {
                out.push_back(fA);
            }
        }
    }
    return out;

}

class SelectorRunner {
    CallGraph& cg;
public:
    SelectorRunner(CallGraph& cg) : cg(cg) {

    }

    FunctionSet run(Selector& selector);
};


#endif //CAPI_SELECTOR_H
