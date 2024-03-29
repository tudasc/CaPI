//
// Created by sebastian on 15.03.22.
//

#include "BasicSelectors.h"
#include <unordered_set>

namespace capi {
bool IncludeListSelector::accept(const CGNode* fNode) {
  return std::find(names.begin(), names.end(), fNode->getName()) != names.end();
}

bool ExcludeListSelector::accept(const CGNode* fNode) {
  return std::find(names.begin(), names.end(), fNode->getName()) == names.end();
}

bool NameSelector::accept(const CGNode* fNode) {
  std::smatch nameMatch;
  bool matches;
  if (isMangled)
    matches = std::regex_match(fNode->getName(), nameMatch, nameRegex);
  else
  {
    std::string functionSubStr = fNode->getFunctionInfo().demangledName.substr(0, fNode->getFunctionInfo().demangledName.find('('));
    matches = std::regex_match(functionSubStr, nameMatch, nameRegex);
  }
  //    if (!matches)
  //       std::cout << fName << " does not match "  << "\n";
  //    else
  //        std::cout << fName << "matches!\n";
  return matches;
}

bool InlineSelector::accept(const CGNode* fNode) {
  if (fNode) {
    return fNode->getFunctionInfo().isInlined;
  }
  return false;
}

bool FilePathSelector::accept(const CGNode* fNode) {
  if (fNode) {

    std::smatch pathMatch;
    bool matches = std::regex_match(fNode->getFunctionInfo().fileName, pathMatch,
                                    nameRegex);
    return matches;
  }
  return false;
}

bool SystemHeaderSelector::accept(const CGNode* fNode) {
  if (fNode) {
    return fNode->getFunctionInfo().definedInSystemInclude;
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

  for (auto& f : in) {
    if (f) {
      if (f->getFunctionInfo().containsPointerCall) {
        out.insert(f);
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

bool MetricSelector::accept(const CGNode* fNode) {
  if (!fNode) {
    return false;
  }
  auto& meta =  fNode->getFunctionInfo().metaData;
  if (meta.is_null()) {
    logError() << "Metadata for function " << fNode->getName() << " not available.\n";
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
    logError() << "Unable to find metadata entry for function " << fNode->getName() << ": " << getFieldsAsString() << "\n";
    return false;
  }

//  bool requiresNumber = Op != MetricCmpOp::StrEquals;

//  int numVal = requiresNumber ? 0 : j.get<int>();

  int fnVal = j.get<int>();

  return evalCmpOp(cmpOp, fnVal, val);
}

FunctionSet CoarseSelector::apply(const FunctionSetList& input) {
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
    bool selected = setContains(in, node);
    bool onlyChild = node->getCallers().size() == 1;
    if (selected) {
      if (mayRemove && onlyChild && !setContains(critical, node)) {
        selected = false;
      } else {
        out.insert(node);
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

FunctionSet MinCallDepthSelector::apply(const FunctionSetList& input) {
  if (input.size() != 1) {
    logError() << "Expected exactly one input set, got " << input.size() << " instead.\n";
    return {};
  }

  FunctionSet in = input.front();

  std::function<int(const CGNode*, std::unordered_set<const CGNode*>)> determineMinDepth = [&](const CGNode* node, std::unordered_set<const CGNode*> visited) -> int {
    auto allCallers = node->findAllCallers();
    std::vector<const CGNode*> filteredCallers;
    for (const auto& caller : allCallers) {
      if (in.find(caller) != in.end()) {
        filteredCallers.push_back(caller);
      }
    }
    if (filteredCallers.empty()) {
      return 0;
    }
    visited.insert(node);
    std::vector<int> parentDepths;
    // Recursively determine minimum depth of all unvisited callers.
    std::transform(filteredCallers.begin(), filteredCallers.end(), std::back_inserter(parentDepths), [&](const CGNode* parent) -> int {
      if (visited.find(parent) == visited.end()) {
        return determineMinDepth(parent, visited);
      }
      return INT32_MAX;
    });
    return *std::min_element(parentDepths.begin(), parentDepths.end()) + 1;
  };

  FunctionSet out;
  for (auto& f : in) {
    int minDepth = determineMinDepth(f, {});
    if (evalCmpOp(op, minDepth, val)) {
      addToSet(out, f);
    }
  }
  return out;
}

//bool MinCallDepthSelector::accept(const CGNode* fNode) {
//  if (!fNode) {
//    return false;
//  }
//
//  std::function<int(const CGNode*, std::unordered_set<const CGNode*>)> determineMinDepth = [&](const CGNode* node, std::unordered_set<const CGNode*> visited) -> int {
//    if (node->getCallers().size() == 0) {
//      return 0;
//    }
//    visited.insert(node);
//    std::vector<int> childDepths;
//    // Recursively determine minimum depth of all unvisited callers.
//    std::transform(node->getCallers().begin(), node->getCallers().end(), std::back_inserter(childDepths), [&](CGNode* child) -> int {
//      if (visited.find(child) == visited.end()) {
//        return determineMinDepth(child, visited);
//      }
//      return INT32_MAX;
//    });
//    return *std::min_element(childDepths.begin(), childDepths.end()) + 1;
//  };
//
//  int result = determineMinDepth(fNode, {});
//
//  //logInfo() << "Min depth of " << fNode->getName() << ": " << result << "\n";
//
//  return evalCmpOp(op, result, val);
//}

}
