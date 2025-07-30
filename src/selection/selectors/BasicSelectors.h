//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_BASICSELECTORS_H
#define CAPI_BASICSELECTORS_H

#include <expected>
#include <iostream>
#include <regex>
#include <optional>

#include "capi/selection/Selector.h"

#include "capi/selection/StatementCountAnalysis.h"
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

enum class CmpOp {
  Smaller, Greater, Equals, NotEquals, EqualsSmaller, EqualsGreater
};

inline std::optional<CmpOp> getCmpOp(const std::string& opStr) {
  CmpOp cmpOp;
  if (opStr == "==") {
    cmpOp = CmpOp::Equals;
  } else if (opStr == "<=") {
    cmpOp = CmpOp::EqualsSmaller;
  }else if (opStr == ">=") {
    cmpOp = CmpOp::EqualsGreater;
  }else if (opStr == "<") {
    cmpOp = CmpOp::Smaller;
  }else if (opStr == ">") {
    cmpOp = CmpOp::Greater;
  }else if (opStr == "!=") {
    cmpOp = CmpOp::NotEquals;
  } else {
    return {};
  }
  return cmpOp;
}

template<typename T>
inline bool evalCmpOp(CmpOp op, T val1, T val2, T eps = 0) {
  T diff = val1 > val2 ? val1 - val2 : val2 - val1;
  switch(op) {
  case CmpOp::Equals:
    return diff <= eps;
  case CmpOp::EqualsGreater:
    return val1 >= val2;
  case CmpOp::EqualsSmaller:
    return val1 <= val2;
  case CmpOp::Greater:
    return val1 > val2;
  case CmpOp::Smaller:
    return val1 < val2;
  case CmpOp::NotEquals:
    return diff > eps;
  default:
    assert("Unhandled cmp op");
  }
}

template<typename DerivedT, typename MDType, typename ValT>
class MetricSelector : public FilterSelector {
public:

 static std::expected<DerivedT, std::string> create(CmpOp op, Param val) {
   if (!isCompatible(val)) {
     return std::unexpected(std::format("Incompatible parameter of type {}", val.kindNames[val.kind]));
   }
   return DerivedT(op, val);
 }

  MetricSelector(std::string selectorName, CmpOp op, Param param) : selectorName(std::move(selectorName)), cmpOp(op), param(param) {}

  static constexpr bool isCompatible(Param param) {
    if (std::is_same_v<ValT, bool>) {
      return param.kind == Param::BOOL;
    }
    if (std::is_integral_v<ValT> || std::is_floating_point_v<ValT>) {
      return param.kind != Param::STRING;
    }
    return false;
  }

  virtual ValT readMetric(MDType& md) = 0;

  bool accept(const metacg::CgNode* fNode) override;

  std::string getName() override {
    return selectorName;
  }

 //  std::string getFieldsAsString() const {
//    std::stringstream ss;
//    for (auto& f : fieldName) {
//      ss << f << ", ";
//    }
//    return ss.str();
//  }

private:
  std::string selectorName;
  CmpOp cmpOp;
  Param param;
};

template<typename DerivedT, typename MDType, typename ValT>
bool MetricSelector<DerivedT,MDType,ValT>::accept(const metacg::CgNode* fNode) {
  if (!fNode) {
    return false;
  }

  if (!fNode->has<MDType>()) {
    logError() << "Metrics metadata " << MDType::key << " for function " << fNode->getFunctionName() << " not available.\n";
    return false;
  }

  auto metricsMD = fNode->get<MDType>();
  assert(metricsMD);

  ValT fnVal = readMetric(*metricsMD);

  // We want to compare as the type given by the input parameter
  switch(param.kind) {
    case Param::BOOL:
      return evalCmpOp<bool>(cmpOp, std::get<bool>(param.val), static_cast<bool>(fnVal));
    case Param::INT:
      return evalCmpOp<long>(cmpOp, std::get<int>(param.val), static_cast<long>(fnVal));
    case Param::FLOAT:
      return evalCmpOp<float>(cmpOp, std::get<float>(param.val), static_cast<float>(fnVal), 1e-12);
    case Param::STRING:
    default:
      assert(false && "Unhandled parameter type");
  }
  return false;
}

class FlopSelector : public MetricSelector<FlopSelector,metacg::NumOperationsMD, int> {
  friend class MetricSelector;
  FlopSelector(CmpOp op, Param val) : MetricSelector("FlopSelector", op, val) {
  }
public:

// static std::expected<FlopSelector, std::string> create(CmpOp op, Param val) {
//   if (!isCompatible(val)) {
//     return std::unexpected(std::format("Incompatible parameter of type {}", val.kindNames[val.kind]));
//   }
//   return FlopSelector(op, val);
// }

  int readMetric(metacg::NumOperationsMD& md) override {
    return md.numberOfFloatOps;
  }
};

class MemOpSelector : public MetricSelector<MemOpSelector, metacg::NumOperationsMD, int> {
  friend class MetricSelector;
  MemOpSelector(CmpOp op, Param val) : MetricSelector("MemOpSelector", op, val) {
  }
public:
// static std::expected<MemOpSelector, std::string> create(CmpOp op, Param val) {
//   if (!isCompatible(val)) {
//     return std::unexpected(std::format("Incompatible parameter of type {}", val.kindNames[val.kind]));
//   }
//   return MemOpSelector(op, val);
// }
  int readMetric(metacg::NumOperationsMD& md) override {
    return md.numberOfMemoryAccesses;
  }
};

class LoopDepthSelector: public MetricSelector<LoopDepthSelector, metacg::LoopDepthMD, int> {
  friend class MetricSelector;
  LoopDepthSelector(CmpOp op, Param val) : MetricSelector("LoopDepthSelector", op, val) {
  }
public:
// static std::expected<LoopDepthSelector, std::string> create(CmpOp op, Param val) {
//   if (!isCompatible(val)) {
//     return std::unexpected(std::format("Incompatible parameter of type {}", val.kindNames[val.kind]));
//   }
//   return LoopDepthSelector(op, val);
// }
  int readMetric(metacg::LoopDepthMD& md) override {
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
  CmpOp op;
  int val;
public:
  MinCallDepthSelector(CmpOp op, int val) : op(op), val(val){
  }

  void init(TraversalHelper &helper) override {
    this->helper = &helper;
  }

  FunctionSet apply(const FunctionSetList& parent) override;

  std::string getName() override {
    return "MinCallDepthSelector";
  }
};

class ISCSelector : public MetricSelector<ISCSelector, ISCMD, long> {
  friend class MetricSelector;
  ISCSelector(CmpOp op, Param val) : MetricSelector("ISCSelector", op, val) {
  }
 public:
  long readMetric(ISCMD& md) override {
    return md.value;
  }
};


}

#endif //CAPI_BASICSELECTORS_H
