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

bool IncludeListSelector::accept(const std::string &fName) {
  return std::find(names.begin(), names.end(), fName) != names.end();
}

bool ExcludeListSelector::accept(const std::string &fName) {
  return std::find(names.begin(), names.end(), fName) == names.end();
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

bool InlineSelector::accept(const std::string &fName) {
  if (auto node = this->cg->get(fName); node) {
    return node->getFunctionInfo().isInlined;
  }
  return false;
}

bool FilePathSelector::accept(const std::string &fName) {
  if (auto node = this->cg->get(fName); node) {

    std::smatch pathMatch;
    bool matches = std::regex_match(node->getFunctionInfo().fileName, pathMatch,
                                    nameRegex);
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

  for (auto &f : in) {
    if (auto *node = cg->get(f); node) {
      if (node->getFunctionInfo().containsPointerCall) {
        out.push_back(f);
      }
    }
  }

  return out;
}
