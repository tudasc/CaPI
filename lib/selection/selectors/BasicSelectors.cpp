//
// Created by sebastian on 15.03.22.
//

#include "BasicSelectors.h"
#include <unordered_set>

namespace capi {
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

bool SystemHeaderSelector::accept(const std::string &fName) {
  if (auto node = this->cg->get(fName); node) {
    return node->getFunctionInfo().definedInSystemInclude;
  }
  return false;
}

FunctionSet UnresolvedCallSelector::apply(const FunctionSetList& input) {
  if (input.size() != 1) {
    logError() << "Expected exactly one input set, got " << input.size() << " instead.\n";
    return {};
  }

  FunctionSet in = input.front();
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

bool evalCmpOp(IntCmpOp op, int val1, int val2) {
  switch(op) {
  case IntCmpOp::Equals:
    return val1 == val2;
  case IntCmpOp::EqualsGreater:
    return val1 >= val2;
  case IntCmpOp::EqualsSmaller:
    return val1 <= val2;
  case IntCmpOp::Greater:
    return val1 > val2;
  case IntCmpOp::Smaller:
    return val1 < val2;
  case IntCmpOp::NotEquals:
    return val1 != val2;
  default:
    assert("Unhandled cmp op");
  }
}

bool MetricSelector::accept(const std::string &fName) {
  auto node = this->cg->get(fName);
  if (!node) {
    return false;
  }
  auto& meta =  node->getFunctionInfo().metaData;
  if (meta.is_null()) {
    logError() << "Metadata for function " << fName << " not available.\n";
    return false;
  }

  json j = meta;

  for (auto& name : fieldName) {
    if (j.is_null())
      break;
    j = j[name];
  }
//  while (!fields.empty()) {
//    auto& name = fields.front();
//    fields.erase(fields.begin());
//    j = j[name];
//    logError() << "Looking for " << name << ": " << j.dump() <<"\n";
//  }

  if (j.is_null()) {
    logError() << "Unable to find metadata entry for function " << fName << ": " << getFieldsAsString() << "\n";
    return false;
  }

//  bool requiresNumber = Op != MetricCmpOp::StrEquals;

//  int numVal = requiresNumber ? 0 : j.get<int>();

  int fnVal = j.get<int>();

  return evalCmpOp(cmpOp, fnVal, val);
}

FunctionSet SparseSelector::apply(const FunctionSetList& input) {
  if (input.size() == 0 || input.size() > 2) {
    logError() << "Expected at least one, and not more than two input sets, got " << input.size() << " instead.\n";
    return {};
  }

  FunctionSet in = input.front();
  FunctionSet critical;

  if (input.size() == 2) {
    critical = input[1];
  }

  FunctionSet out;

  std::unordered_set<const CGNode*> visited;

  // Remove functions that fulfill all of the following conditions:
  // - They have exactly one caller
  // - Their caller is either selected or has itself only one caller
  // - They are not in the list of critical functions

  std::function<void(const CGNode*, bool)> traverse = [&](const CGNode* node, bool mayRemove) {
    visited.insert(node);
    bool selected = setContains(in, node->getName());
    bool onlyChild = node->getCallers().size() == 1;
    if (selected) {
      if (mayRemove && onlyChild && !setContains(critical, node->getName())) {
        selected = false;
      } else {
        out.push_back(node->getName());
      }
    }

    for (auto& callee : node->getCallees()) {
      if (visited.find(callee) == visited.end()) {
        // Callees are eligible for removal, if (1) their parent was selected or (2) their parent is an only child.
        traverse(callee, selected ||  onlyChild);
      }
    }

  };

  std::vector<CGNode*> selectionRoots;
  for (auto root : cg->findRoots()) {
    traverse(root, false);
  }

  return out;
}

bool MinCallDepthSelector::accept(const std::string &fName) {
  auto node = this->cg->get(fName);
  if (!node) {
    return false;
  }

  std::function<int(CGNode*, std::unordered_set<CGNode*>)> determineMinDepth = [&](CGNode* node, std::unordered_set<CGNode*> visited) -> int {
    if (node->getCallers().size() == 0) {
      return 0;
    }
    visited.insert(node);
    std::vector<int> childDepths;
    // Recursively determine minimum depth of all unvisited callers.
    std::transform(node->getCallers().begin(), node->getCallers().end(), std::back_inserter(childDepths), [&](CGNode* child) -> int {
      if (visited.find(child) == visited.end()) {
        return determineMinDepth(child, visited);
      }
      return INT32_MAX;
    });
    return *std::min_element(childDepths.begin(), childDepths.end()) + 1;
  };

  int result = determineMinDepth(node, {});

  //logInfo() << "Min depth of " << fName << ": " << result << "\n";

  return evalCmpOp(op, result, val);
}

}
