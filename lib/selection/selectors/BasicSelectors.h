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

enum class IntCmpOp {
  Smaller, Greater, Equals, NotEquals, EqualsSmaller, EqualsGreater
};

inline std::optional<IntCmpOp> getCmpOp(const std::string& opStr) {
  IntCmpOp cmpOp;
  if (opStr == "==") {
    cmpOp = IntCmpOp::Equals;
  } else if (opStr == "<=") {
    cmpOp = IntCmpOp::EqualsSmaller;
  }else if (opStr == ">=") {
    cmpOp = IntCmpOp::EqualsGreater;
  }else if (opStr == "<") {
    cmpOp = IntCmpOp::Smaller;
  }else if (opStr == ">") {
    cmpOp = IntCmpOp::Greater;
  }else if (opStr == "!=") {
    cmpOp = IntCmpOp::NotEquals;
  } else {
    return {};
  }
  return cmpOp;
}

class MetricSelector : public FilterSelector {
public:

  MetricSelector(std::string selectorName, std::vector<std::string> fieldName,
                 IntCmpOp op, int val) : selectorName(std::move(selectorName)), fieldName(std::move(fieldName)), cmpOp(op), val(val){
  }

  bool accept(const std::string &fName) override;

  std::string getName() override {
    return selectorName;
  }

private:
  std::string getFieldsAsString() const {
    std::stringstream ss;
    for (auto& f : fieldName) {
      ss << f << ", ";
    }
    return ss.str();
  }

private:
  std::string selectorName;
  std::vector<std::string> fieldName;
  IntCmpOp cmpOp;
  int val;
};

class FlopSelector : public MetricSelector {
public:
  FlopSelector(IntCmpOp op, int val) : MetricSelector("FlopSelector", {"numOperations", "numberOfFloatOps"}, op, val) {
  }
};

class MemOpSelector : public MetricSelector {
public:
  MemOpSelector(IntCmpOp op, int val) : MetricSelector("MemOpSelector", {"numOperations", "numberOfMemoryAccesses"}, op, val) {
  }
};

class LoopDepthSelector: public MetricSelector {
public:
  LoopDepthSelector(IntCmpOp op, int val) : MetricSelector("LoopDepthSelector", {"loopDepth"}, op, val) {
  }
};

class SparseSelector : public Selector {
  CallGraph *cg;

public:
  explicit SparseSelector() {
  }

  void init(CallGraph &cg) override {
    this->cg = &cg;
  }

  FunctionSet apply(const FunctionSetList& input) override;


  std::string getName() override {
    return "SparseSelector";
  }
};

class MinCallDepthSelector : public FilterSelector {
  IntCmpOp op;
  int val;
public:
  MinCallDepthSelector(IntCmpOp op, int val) : op(op), val(val){
  }

  bool accept(const std::string &fName) override;

  std::string getName() override {
    return "MinCallDepthSelector";
  }
};



}

#endif //CAPI_BASICSELECTORS_H
