//
// Created by sebastian on 28.10.21.
//

#include "Selector.h"

#include <iostream>
#include <set>

#include "CallGraph.h"

FunctionSet SelectorRunner::run(Selector &selector) {
    selector.init(cg);
    return selector.apply();
}

bool WhiteListSelector::accept(const std::string &fName) {
    return std::find(names.begin(), names.end(), fName) != names.end();
}

bool NameSelector::accept(const std::string &fName) {
    std::smatch nameMatch;
    bool matches = std::regex_match(fName, nameMatch, nameRegex);
//    if (!matches)
//       std::cout << fName << " does not match "  << "\n";
//    else
//        std::cout << fName << "matches!\n";
    return matches;
}

bool FilePathSelector::accept(const std::string &fName) {
    if (auto node = this->cg->get(fName); node) {

        std::smatch pathMatch;
        bool matches = std::regex_match(node->getFunctionInfo().fileName, pathMatch, nameRegex);
        return matches;
    }
    return false;
}


bool SystemIncludeSelector::accept(const std::string &fName) {
    if (auto node = this->cg->get(fName); node) {
        return node->getFunctionInfo().definedInSystemInclude;
    }
    return false;
}

FunctionSet UnresolvedCallSelector::apply() {
    FunctionSet in = input->apply();
    FunctionSet out;

    for (auto& f : in) {
        if (auto* node = cg->get(f); node) {
            if (node->getFunctionInfo().containsPointerCall) {
                out.push_back(f);
            }
        }
    }

    return out;
}

///**
// * Traverses the call chain upwards, calling the given visit function on each node.
// * @tparam VisitFn Function that takes a CGNode& argument.
// * @param node
// * @param visit
// * @returns The number of visited functions.
// */
//template<typename VisitFn>
//int traverseCallers(CGNode& node, VisitFn&& visit) {
//    std::vector<CGNode*> workingSet;
//    std::vector<CGNode*> alreadyVisited;
//
//    workingSet.push_back(&node);
//
//    do {
//        auto currentFn = workingSet.back();
//        workingSet.pop_back();
////        std::cout << "Visiting caller " << currentFn->getName() << "\n";
//        visit(*currentFn);
//        alreadyVisited.push_back(currentFn);
//        for (auto& caller : currentFn->getCallers()) {
//            if (std::find(workingSet.begin(), workingSet.end(), caller) == workingSet.end() &&
//                std::find(alreadyVisited.begin(), alreadyVisited.end(), caller) == alreadyVisited.end()) {
//                workingSet.push_back(caller);
//            }
//        }
//
//    } while(!workingSet.empty());
//
//    return alreadyVisited.size();
//}




