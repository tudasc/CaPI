//
// Created by sebastian on 28.10.21.
//

#ifndef CAPI_SELECTOR_H
#define CAPI_SELECTOR_H

#include <iostream>
#include <unordered_set>
#include <variant>

#include "TraversalHelper.h"
//#include "MetaCGReader.h"
#include "support/Logging.h"

#include "Callgraph.h"


namespace capi {

struct Param {

  using ParamVal = std::variant<bool, int, float, std::string>;

  enum Kind {
    INT, FLOAT, BOOL, STRING
  };

  const char* kindNames[4] = {"INT", "FLOAT", "BOOL", "STRING"};

  Kind kind;

  ParamVal val;

  static Param makeInt(int val) {
    return {INT, val};
  }

  static Param makeFloat(float val) {
    return {FLOAT, val};
  }

  static Param makeBool(bool val) {
    return {BOOL, val};
  }

  static Param makeString(std::string val) {
    return {STRING, val};
  }

 private:
  Param() = default;
  Param(Kind kind, ParamVal val) : kind(kind), val(val){}
};


using FunctionSet = std::unordered_set<const metacg::CgNode*>;

using FunctionSetList = std::vector<FunctionSet>;

template<typename T>
inline bool setContains(const std::vector<T>& set, const T& entry) {
  return std::find(set.begin(), set.end(), entry) != set.end();
}

template<typename T>
inline bool setContains(const std::unordered_set<T>& set, const T& entry) {
  return set.find(entry) != set.end();
}

template<typename T>
inline std::vector<T> intersect(const std::vector<T>& setA, const std::vector<T>& setB) {
  std::vector<T> result;
  for (auto& entry : setA) {
    if (setContains(setB, entry))
      result.push_back(entry);
  }
  return result;
}

template<typename T>
inline bool addToSet(std::vector<T>& set, const T& entry) {
  if (!setContains(set, entry)) {
    set.push_back(entry);
    return true;
  }
  return false;
}

template<typename T>
inline bool addToSet(std::unordered_set<T>& set, const T& entry) {
  if (!setContains(set, entry)) {
    set.insert(entry);
    return true;
  }
  return false;
}

// TODO: Incomplete
//class AnalysisManager {
// public:
//  using AnalysisID = int;
//
//  AnalysisManager(TraversalHelper& helper) : helper(helper) {}
//
//  template<typename AnalysisT>
//  AnalysisT::AnalysisResultT& getAnalysisResult() {
//    auto* result = resultMap[AnalysisT::getID()];
//    if (!result) {
//      AnalysisT analysis;
//      result = analysis.run(helper);
//    }
//    return *result;
//  }
//
// private:
//  TraversalHelper& helper;
//  std::unordered_map<AnalysisID, void*> resultMap;
//
//};

class Selector
{
public:

  virtual ~Selector() = default;

  virtual void init(TraversalHelper& helper)
  {}

  virtual FunctionSet apply(const FunctionSetList &) = 0;

  virtual std::string getName() = 0;
};

using SelectorPtr = std::unique_ptr<Selector>;


class FilterSelector : public Selector
{

protected:
  TraversalHelper *helper;

public:
  FilterSelector() = default;

  void init(TraversalHelper& helper) override
  {
    this->helper = &helper;
  }

  virtual bool accept(const metacg::CgNode*) = 0;

  FunctionSet apply(const FunctionSetList &input) override
  {
    if (input.size() != 1) {
      logError() << "Expected exactly one input set, got " << input.size() << " instead.\n";
      return {};
    }
    FunctionSet in = input.front();
    //        std::cout << "Input contains " << in.size() << " elements\n";
    for (auto it = in.begin(); it != in.end();) {
      if (!accept(*it))
        in.erase(it++);
      else
        ++it;
    }

    return in;
  }
};

}

#endif // CAPI_SELECTOR_H
