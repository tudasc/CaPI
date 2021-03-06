//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_BASICSELECTORS_H
#define CAPI_BASICSELECTORS_H

#include <iostream>
#include <regex>
#include <optional>

#include "Selector.h"


namespace capi {

class EverythingSelector : public Selector {
  FunctionSet allFunctions;

public:
  void init(CallGraph &cg) override {
    for (auto &node : cg.getNodes()) {
      allFunctions.push_back(node->getName());
    }
  }

  FunctionSet apply(const FunctionSetList&) override { return allFunctions; }

  std::string getName() override {
    return "EverythingSelector";
  }
};


class IncludeListSelector : public FilterSelector {
  std::vector<std::string> names;

public:
  explicit IncludeListSelector(std::vector<std::string> names)
          : names(names) {}

  bool accept(const std::string &fName) override;

  std::string getName() override {
    return "include";
  }
};

class ExcludeListSelector : public FilterSelector {
  std::vector<std::string> names;

public:
  ExcludeListSelector(std::vector<std::string> names)
          :  names(std::move(names)) {}

  bool accept(const std::string &fName) override;

  std::string getName() override {
    return "exclude";
  }
};

class NameSelector : public FilterSelector {
  std::regex nameRegex;

public:
  NameSelector(std::string regexStr): nameRegex(regexStr) {}

  bool accept(const std::string &fName) override;

  std::string getName() override {
    return "NameSelector";
  }
};

class InlineSelector : public FilterSelector {
public:
  InlineSelector() = default;

  bool accept(const std::string &fName) override;

  std::string getName() override {
    return "InlineSelector";
  }
};

class FilePathSelector : public FilterSelector {
  std::regex nameRegex;

public:
  FilePathSelector(std::string regexStr)
          : nameRegex(regexStr) {}

  bool accept(const std::string &fName) override;

  std::string getName() override {
    return "FilePathSelector";
  }
};

class SystemHeaderSelector : public FilterSelector {
public:
  SystemHeaderSelector() = default;

  bool accept(const std::string &fName) override;

  std::string getName() override {
    return "SystemHeaderSelector";
  }
};

class UnresolvedCallSelector : public Selector {
  CallGraph *cg;

public:
  UnresolvedCallSelector() = default;

  void init(CallGraph &cg) override {
    this->cg = &cg;
  }

  FunctionSet apply(const FunctionSetList& input) override;

  std::string getName() override {
    return "UnresolvedCallSelector";
  }
};

enum class MetricCmpOp {
  Smaller, Greater, Equals, NotEquals, EqualsSmaller, EqualsGreater
};

inline std::optional<MetricCmpOp> getCmpOp(const std::string& opStr) {
  MetricCmpOp cmpOp;
  if (opStr == "==") {
    cmpOp = MetricCmpOp::Equals;
  } else if (opStr == "<=") {
    cmpOp = MetricCmpOp::EqualsSmaller;
  }else if (opStr == ">=") {
    cmpOp = MetricCmpOp::EqualsGreater;
  }else if (opStr == "<") {
    cmpOp = MetricCmpOp::Smaller;
  }else if (opStr == ">") {
    cmpOp = MetricCmpOp::Greater;
  }else if (opStr == "!=") {
    cmpOp = MetricCmpOp::NotEquals;
  } else {
    return {};
  }
  return cmpOp;
}

class MetricSelector : public FilterSelector {
public:

  MetricSelector(std::string selectorName, std::vector<std::string> fieldName, MetricCmpOp op, int val) : selectorName(std::move(selectorName)), fieldName(std::move(fieldName)), cmpOp(op), val(val){
  }

  bool accept(const std::string &fName) override;

  std::string getName() override {
    return selectorName;
  }

private:
  std::string selectorName;
  std::vector<std::string> fieldName;
  MetricCmpOp cmpOp;
  int val;
};

class FlopSelector : public MetricSelector {
public:
  FlopSelector(MetricCmpOp op, int val) : MetricSelector("FlopSelector", {"numOperations", "numberOfFloatOps"}, op, val) {
  }
};

class MemOpSelector : public MetricSelector {
public:
  MemOpSelector(MetricCmpOp op, int val) : MetricSelector("MemOpSelector", {"numOperations", "numberOfMemoryAccesses"}, op, val) {
  }
};

class LoopDepthSelector: public MetricSelector {
public:
  LoopDepthSelector(MetricCmpOp op, int val) : MetricSelector("LoopDepthSelector", {"loopDepth"}, op, val) {
  }
};



}

#endif //CAPI_BASICSELECTORS_H
