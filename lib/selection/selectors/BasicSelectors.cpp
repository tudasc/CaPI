//
// Created by sebastian on 15.03.22.
//

#include "BasicSelectors.h"

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

  int numVal = j.get<int>();

  switch(cmpOp) {
  case MetricCmpOp::Equals:
    return numVal == val;
  case MetricCmpOp::EqualsGreater:
    return numVal >= val;
  case MetricCmpOp::EqualsSmaller:
    return numVal <= val;
  case MetricCmpOp::Greater:
    return numVal > val;
  case MetricCmpOp::Smaller:
    return numVal < val;
  case MetricCmpOp::NotEquals:
    return numVal != val;
  default:
    assert("Unhandled cmp op");
    break;
  }
  return false;
}

}
