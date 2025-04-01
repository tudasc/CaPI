//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_BASICSELECTORS_H
#define CAPI_BASICSELECTORS_H

#include <iostream>
#include <regex>
#include <optional>

#include "Selector.h"

#include "metadata/BuiltinMD.h"
#include "metadata/NumOperationsMD.h"


namespace capi {

class EverythingSelector : public Selector {
  FunctionSet allFunctions;

public:
  void init(TraversalHelper &helper) override {
    for (auto& [id, node] : helper.cg.getNodes()) {
      allFunctions.insert(node.get());
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

  bool accept(const metacg::CgNode* fNode) override;

  std::string getName() override {
    return "include";
  }
};

class ExcludeListSelector : public FilterSelector {
  std::vector<std::string> names;

public:
  ExcludeListSelector(std::vector<std::string> names)
          :  names(std::move(names)) {}

  bool accept(const metacg::CgNode* fNode) override;

  std::string getName() override {
    return "exclude";
  }
};

class NameSelector : public FilterSelector {
  std::regex nameRegex;
  std::vector<std::regex> parameterRegexes;
  bool isMangled;
  bool isEmptyMatching{false};

public:
  NameSelector(std::string regexStr, std::vector<std::string>& parameterRegexStrings, bool isMangled)
      : isMangled(isMangled)
  {
      nameRegex = std::regex(isMangled ? regexStr.substr(1, regexStr.size()) : regexStr);

      for (auto& param : parameterRegexStrings) {
        parameterRegexes.emplace_back(param);
	if (param == "()")
         isEmptyMatching = true;
      }
  }

  bool accept(const metacg::CgNode* fNode) override;

  std::string getName() override {
    return "NameSelector";
  }
};

class InlineSelector : public FilterSelector {
public:
  InlineSelector() = default;

  bool accept(const metacg::CgNode* fNode) override;

  std::string getName() override {
    return "InlineSelector";
  }
};

class FilePathSelector : public FilterSelector {
  std::regex nameRegex;

public:
  FilePathSelector(std::string regexStr)
          : nameRegex(regexStr) {}

  bool accept(const metacg::CgNode* fNode) override;

  std::string getName() override {
    return "FilePathSelector";
  }
};

class SystemHeaderSelector : public FilterSelector {
public:
  SystemHeaderSelector() = default;

  bool accept(const metacg::CgNode* fNode) override;

  std::string getName() override {
    return "SystemHeaderSelector";
  }
};

class UnresolvedCallSelector : public Selector {
  TraversalHelper *helper{nullptr};

public:
  UnresolvedCallSelector() = default;

  void init(TraversalHelper &helper) override {
    this->helper = &helper;
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

inline bool evalCmpOp(IntCmpOp op, int val1, int val2) {
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

template<typename MDType>
class MetricSelector : public FilterSelector {
public:

  MetricSelector(std::string selectorName, IntCmpOp op, int val) : selectorName(std::move(selectorName)), cmpOp(op), val(val){
  }

  virtual long readMetric(MDType& md) = 0;

  bool accept(const metacg::CgNode* fNode) override;

  std::string getName() override {
    return selectorName;
  }

private:
//  std::string getFieldsAsString() const {
//    std::stringstream ss;
//    for (auto& f : fieldName) {
//      ss << f << ", ";
//    }
//    return ss.str();
//  }

private:
  std::string selectorName;
  IntCmpOp cmpOp;
  int val;
};

template<typename T>
bool MetricSelector<T>::accept(const metacg::CgNode* fNode) {
  if (!fNode) {
    return false;
  }

  if (!fNode->has<T>()) {
    logError() << "Metrics metadata " << T::key << " for function " << fNode->getFunctionName() << " not available.\n";
    return false;
  }

  auto metricsMD = fNode->get<T>();
  assert(metricsMD);

  long fnVal = readMetric(*metricsMD);

  return evalCmpOp(cmpOp, fnVal, val);
}

class FlopSelector : public MetricSelector<NumOperationsMD> {
public:
  FlopSelector(IntCmpOp op, int val) : MetricSelector("FlopSelector", op, val) {
  }

  long readMetric(NumOperationsMD& md) override {
    return md.numberOfFloatOps;
  }
};

class MemOpSelector : public MetricSelector<NumOperationsMD> {
public:
  MemOpSelector(IntCmpOp op, int val) : MetricSelector("MemOpSelector", op, val) {
  }
  long readMetric(NumOperationsMD& md) override {
    return md.numberOfMemoryAccesses;
  }
};

class LoopDepthSelector: public MetricSelector<LoopDepthMD> {
public:
  LoopDepthSelector(IntCmpOp op, int val) : MetricSelector("LoopDepthSelector", op, val) {
  }
  long readMetric(LoopDepthMD& md) override {
    return md.loopDepth;
  }
};

class CoarseSelector : public Selector {
  TraversalHelper *helper{nullptr};

public:
  explicit CoarseSelector() {
  }

  void init(TraversalHelper &helper) override {
    this->helper = &helper;
  }

  FunctionSet apply(const FunctionSetList& input) override;


  std::string getName() override {
    return "CoarseSelector";
  }
};

class MinCallDepthSelector : public Selector {
  TraversalHelper *helper{nullptr};
  IntCmpOp op;
  int val;
public:
  MinCallDepthSelector(IntCmpOp op, int val) : op(op), val(val){
  }

  void init(TraversalHelper &helper) override {
    this->helper = &helper;
  }

  FunctionSet apply(const FunctionSetList&parent) override;

  std::string getName() override {
    return "MinCallDepthSelector";
  }
};



}

#endif //CAPI_BASICSELECTORS_H
