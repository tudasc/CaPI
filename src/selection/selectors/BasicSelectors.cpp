//
// Created by sebastian on 15.03.22.
//

#include "BasicSelectors.h"

#include <unordered_set>

#include "capi/selection/metadata/CaPIMD.h"
#include "metacg/metadata/BuiltinMD.h"

namespace capi {
bool IncludeListSelector::accept(const metacg::CgNode* fNode) {
  return std::find(names.begin(), names.end(), fNode->getFunctionName()) != names.end();
}

bool ExcludeListSelector::accept(const metacg::CgNode* fNode) {
  return std::find(names.begin(), names.end(), fNode->getFunctionName()) == names.end();
}

bool NameSelector::accept(const metacg::CgNode* fNode) {
  std::smatch nameMatch;
  bool matches;
  if (isMangled) {
    auto name = fNode->getFunctionName();
    matches = std::regex_match(name, nameMatch, nameRegex);
  } else {
    if (!fNode->has<CaPIMD>()) {
      logError() << "Could not run NameSelector: demangled names not available!";
      return false;
    }
    auto& info = fNode->get<CaPIMD>()->value;
    if ((matches = std::regex_match(info.demangledName, nameMatch, nameRegex))) {
      if (!parameterRegexes.empty()) {
        if (isEmptyMatching)
          matches = info.parameters.empty();
        else if ((matches = (info.parameters.size() == parameterRegexes.size()))) {
          for (int i = 0; i < info.parameters.size(); i++) {
            if (!(matches = std::regex_match(info.parameters[i], nameMatch, parameterRegexes[i])))
              break;
          }
        }
      }
    }
  }

  return matches;
}

bool InlineSelector::accept(const metacg::CgNode* fNode) {
  if (!fNode) {
    return false;
  }
  if (!fNode->has<metacg::InlineMD>()) {
    return false;
  }
  auto& md = *fNode->get<metacg::InlineMD>();
  return md.isMarkedInline() || md.isMarkedAlwaysInline() || md.isTemplate();
}

bool FilePathSelector::accept(const metacg::CgNode* fNode) {
  if (fNode) {

    std::smatch pathMatch;
    auto path = fNode->getOrigin();
    if (!path) {
      return false;
    }

    bool matches = std::regex_match(*path, pathMatch,
                                    nameRegex);
    return matches;
  }
  return false;
}

bool SystemHeaderSelector::accept(const metacg::CgNode* fNode) {
  if (!fNode) {
    return false;
  }
  if (!fNode->has<metacg::FilePropertiesMD>()) {
    return false;
  }
  auto& md = *fNode->get<metacg::FilePropertiesMD>();
  return md.fromSystemInclude;
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
//      if (f->getFunctionInfo().containsPointerCall) {
      // FIXME: pointer call MD
      if (false) {
        out.insert(f);
      }
    }
  }

  return out;
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

  std::unordered_set<const metacg::CgNode*> visited;

  // Remove functions that fulfill all of the following conditions:
  // - They have exactly one caller
  // - Their caller is either selected or has itself only one caller
  // - They are not in the list of critical functions

  std::function<void(const metacg::CgNode*, bool)> traverse = [&](const metacg::CgNode* node, bool mayRemove) {
    visited.insert(node);
    bool selected = setContains(in, node);
    bool onlyChild = helper->cg.getCallers(*node).size() == 1;
    if (selected) {
      if (mayRemove && onlyChild && !setContains(critical, node)) {
        selected = false;
      } else {
        out.insert(node);
      }
    }

    for (auto& callee : helper->cg.getCallees(*node)) {
      if (visited.find(callee) == visited.end()) {
        // Callees are eligible for removal, if (1) their parent was selected or (2) their parent is an only child.
        traverse(callee, selected ||  onlyChild);
      }
    }

  };

  for (auto root : helper->findRoots()) {
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

  std::function<int(const metacg::CgNode*, std::unordered_set<const metacg::CgNode*>)> determineMinDepth = [&](const metacg::CgNode* node, std::unordered_set<const metacg::CgNode*> visited) -> int {
    auto allCallers = helper->get(node).findAllCallers();
    std::vector<const metacg::CgNode*> filteredCallers;
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
    std::transform(filteredCallers.begin(), filteredCallers.end(), std::back_inserter(parentDepths), [&](const metacg::CgNode* parent) -> int {
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

//bool MinCallDepthSelector::accept(const metacg::CgNode* fNode) {
//  if (!fNode) {
//    return false;
//  }
//
//  std::function<int(const metacg::CgNode*, std::unordered_set<const metacg::CgNode*>)> determineMinDepth = [&](const metacg::CgNode* node, std::unordered_set<const metacg::CgNode*> visited) -> int {
//    if (node->getCallers().size() == 0) {
//      return 0;
//    }
//    visited.insert(node);
//    std::vector<int> childDepths;
//    // Recursively determine minimum depth of all unvisited callers.
//    std::transform(node->getCallers().begin(), node->getCallers().end(), std::back_inserter(childDepths), [&](metacg::CgNode* child) -> int {
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
