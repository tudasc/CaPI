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
    for (auto& node : helper.cg.getNodes()) {
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

template<typename MetricT, typename MDType, typename ValT>
class SimpleMDMetric {
 public:

  using ValType = ValT;

  static std::expected<ValT, std::string> getValue(const metacg::CgNode* fNode) {
    if (!fNode) {
      return std::unexpected("Node is null");
    }

    if (!fNode->has<MDType>()) {
      return std::unexpected(std::format("Metrics metadata {} not available for function {}.", MDType::key, fNode->getFunctionName()));
    }

    auto metricsMD = fNode->get<MDType>();
    assert(metricsMD);
    return MetricT::readMDVal(*metricsMD);
  }

};

template<typename MetricT1, typename MetricT2, typename ValT, auto op>
class DerivedMetric {
 public:

  using ValType = ValT;

  static constexpr std::string_view Name = "DerivedMetric";

  static std::expected<ValT, std::string> getValue(const metacg::CgNode* fNode) {
    auto v1 = MetricT1::getValue(fNode);
    auto v2 = MetricT2::getValue(fNode);
    if (!v1 || !v2) {
      return std::unexpected(std::format("Failed to read derived metric: {} {}", v1.error_or(""), v2.error_or("")));
    }
    return static_cast<ValT>(op(v1.value(), v2.value()));
  }
};

template<typename MetricT>
class MetricSelector : public FilterSelector {
public:

 using ValType = MetricT::ValType;

 static std::expected<MetricSelector, std::string> create(const std::string& selectorName, CmpOp op, Param val) {
   if (!isCompatible(val)) {
     return std::unexpected(std::format("Incompatible parameter of type {}", val.kindNames[val.kind]));
   }
   return MetricSelector(selectorName, op, val);
 }

 static std::expected<MetricSelector, std::string> create(CmpOp op, Param val) {
   std::string name = std::format("{}Selector", MetricT::Name);
   return create(name, op, val);
 }

  MetricSelector(const std::string& selectorName, CmpOp op, Param param) : selectorName(selectorName), cmpOp(op), param(param) {}

  static constexpr bool isCompatible(Param param) {
    if (std::is_same_v<ValType, bool>) {
      return param.kind == Param::BOOL;
    }
    if (std::is_integral_v<ValType> || std::is_floating_point_v<ValType>) {
      return param.kind != Param::STRING;
    }
    return false;
  }

  bool accept(const metacg::CgNode* fNode) override;

  std::string getName() override {
    return selectorName;
  }

private:
  std::string selectorName;
  CmpOp cmpOp;
  Param param;
};

template<typename MetricT>
bool MetricSelector<MetricT>::accept(const metacg::CgNode* fNode) {
  auto valOrErr = MetricT::getValue(fNode);
  if (!valOrErr) {
    logError() << valOrErr.error() << "\n";
    return false;
  }
  ValType fnVal = valOrErr.value();

  // We want to compare as the type given by the input parameter
  switch(param.kind) {
    case Param::BOOL:
      return evalCmpOp<bool>(cmpOp, static_cast<bool>(fnVal), std::get<bool>(param.val));
    case Param::INT:
      return evalCmpOp<long>(cmpOp, static_cast<long>(fnVal), std::get<int>(param.val));
    case Param::FLOAT:
      return evalCmpOp<float>(cmpOp, static_cast<float>(fnVal), std::get<float>(param.val), 1e-12);
    case Param::STRING:
    default:
      assert(false && "Unhandled parameter type");
  }
  return false;
}

class FlopMetric : public SimpleMDMetric<FlopMetric, metacg::NumOperationsMD, int> {
 public:
  static constexpr std::string_view Name = "FlopMetric";
  static int readMDVal(const metacg::NumOperationsMD& md) { return md.numberOfFloatOps; }
};

class MemOpMetric : public SimpleMDMetric<MemOpMetric, metacg::NumOperationsMD, int> {
 public:
  static constexpr std::string_view Name = "MemOpMetric";
  static int readMDVal(const metacg::NumOperationsMD& md) {
    return md.numberOfMemoryAccesses;
  }
};

class LoopDepthMetric: public SimpleMDMetric<LoopDepthMetric, metacg::LoopDepthMD, int> {
 public:
  static constexpr std::string_view Name = "LoopDepthMetric";
  static int readMDVal(const metacg::LoopDepthMD& md) {
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

class ISCMetric : public SimpleMDMetric<ISCMetric, ISCMD, long> {
 public:
  static constexpr std::string_view Name = "ISCMetric";
  static long readMDVal(const ISCMD& md) {
    return md.value;
  }
};

class IICMetric : public SimpleMDMetric<IICMetric, IICMD, long> {
public:
    static constexpr std::string_view Name = "IICMetric";
    static long readMDVal(const IICMD& md) {
        return md.value;
    }


    static inline constexpr std::array<std::string_view, 3> requiredAnalyses {
            "instructionCount"
    };

};


}

#endif //CAPI_BASICSELECTORS_H
