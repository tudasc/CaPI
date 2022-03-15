//
// Created by sebastian on 28.10.21.
//

#ifndef CAPI_SELECTOR_H
#define CAPI_SELECTOR_H

#include <iostream>
#include <regex>

#include "CallGraph.h"
#include "MetaCGReader.h"
#include "Utils.h"

using FunctionSet = std::vector<std::string>;

using FunctionSetList = std::vector<FunctionSet>;

class Selector {
public:
  virtual void init(CallGraph &cg) {}

  virtual FunctionSet apply(const FunctionSetList&) = 0;

  virtual std::string getName() = 0;
};

using SelectorPtr = std::unique_ptr<Selector>;

class EverythingSelector : public Selector {
  FunctionSet allFunctions;

public:
  void init(CallGraph &cg) override {
    for (auto &node : cg.getNodes()) {
      allFunctions.push_back(node->getName());
    }
  }

  FunctionSet apply(const FunctionSetList&) override { return allFunctions; }

  std::string getName() {
    return "%%";
  }
};

class FilterSelector : public Selector {

protected:
  CallGraph *cg;

public:
  FilterSelector() = default;

  void init(CallGraph &cg) override {
    this->cg = &cg;
  }

  virtual bool accept(const std::string &) = 0;

  FunctionSet apply(const FunctionSetList& input) override {
    if (input.size() != 1) {
      logError() << "Expected exactly one input set, got " << input.size() << "instead.\n";
      return {};
    }
    FunctionSet in = input.front();
    //        std::cout << "Input contains " << in.size() << " elements\n";
    in.erase(std::remove_if(
                 in.begin(), in.end(),
                 [this](std::string &fName) -> bool { return !accept(fName); }),
             in.end());
    return in;
  }
};

enum class SetOperation { UNION, INTERSECTION, COMPLEMENT };

template <SetOperation Op> class SetOperationSelector : public Selector {
public:
  SetOperationSelector() = default;

  void init(CallGraph &cg) override {
  }

  FunctionSet apply(const FunctionSetList& input) override;

  std::string getName() override {
    switch(Op) {
    case SetOperation::UNION:
      return "join";
    case SetOperation::INTERSECTION:
      return "intersect";
    case SetOperation::COMPLEMENT:
      return "subtract";
    }
    return "invalid";
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
    return "byName";
  }
};

class InlineSelector : public FilterSelector {
public:
  InlineSelector() = default;

  bool accept(const std::string &fName) override;

  std::string getName() override {
    return "isInlined";
  }
};

class FilePathSelector : public FilterSelector {
  std::regex nameRegex;

public:
  FilePathSelector(std::string regexStr)
      : nameRegex(regexStr) {}

  bool accept(const std::string &fName) override;

  std::string getName() override {
    return "byPath";
  }
};

class SystemIncludeSelector : public FilterSelector {
  std::regex nameRegex;

public:
  SystemIncludeSelector() = default;

  bool accept(const std::string &fName) override;

  std::string getName() override {
    return "isInSystemHeader";
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
    return "isUnresolved";
  }
};

enum class TraverseDir { TraverseUp, TraverseDown };

template <TraverseDir dir> class CallPathSelector : public Selector {
  CallGraph *cg;

public:
  CallPathSelector() = default;

  void init(CallGraph &cg) override {
    this->cg = &cg;
  }

  FunctionSet apply(const FunctionSetList& input) override;

  std::string getName() override {
    if constexpr (dir == TraverseDir::TraverseUp) {
      return "onCallPathTo";
    }
    return "onCallPathFrom";
  }
};

/**
 * Traverses the call chain downwards, calling the given visit function on each
 * node.
 * @tparam VisitFn Function that takes a CGNode& argument and returns the next
 * nodes to traverse.
 * @tparam VisitFn Function that takes a CGNode& argument.
 * @param node
 * @param visit
 * @returns The number of visited functions.
 */
template <typename TraverseFn, typename VisitFn>
int traverseCallGraph(CGNode &node, TraverseFn &&selectNextNodes,
                      VisitFn &&visit) {
  std::vector<CGNode *> workingSet;
  std::vector<CGNode *> alreadyVisited;

  workingSet.push_back(&node);

  do {
    auto currentNode = workingSet.back();
    workingSet.pop_back();
    //        std::cout << "Visiting caller " << currentNode->getName() << "\n";
    visit(*currentNode);
    alreadyVisited.push_back(currentNode);
    for (auto &nextNode : selectNextNodes(*currentNode)) {
      if (std::find(workingSet.begin(), workingSet.end(), nextNode) ==
              workingSet.end() &&
          std::find(alreadyVisited.begin(), alreadyVisited.end(), nextNode) ==
              alreadyVisited.end()) {
        workingSet.push_back(nextNode);
      }
    }

  } while (!workingSet.empty());

  return alreadyVisited.size();
}

template <TraverseDir Dir> FunctionSet CallPathSelector<Dir>::apply(const FunctionSetList& input) {
  static_assert(Dir == TraverseDir::TraverseDown ||
                Dir == TraverseDir::TraverseUp);

  if (input.size() != 1) {
    logError() << "Expected exactly one input sets, got " << input.size() << "instead.\n";
    return {};
  }


  FunctionSet in = input.front();
  FunctionSet out(in);

  auto visitFn = [&out](CGNode &node) {
    if (std::find(out.begin(), out.end(), node.getName()) == out.end()) {
      out.push_back(node.getName());
    }
  };

  for (auto &fn : in) {
    auto fnNode = cg->get(fn);

    if constexpr (Dir == TraverseDir::TraverseDown) {
      int count = traverseCallGraph(
          *fnNode, [](CGNode & node) -> auto { return node.getCallees(); },
          visitFn);
      std::cout << "Functions on call path from " << fn << ": " << count << "\n";
    } else if constexpr (Dir == TraverseDir::TraverseUp) {
      int count = traverseCallGraph(
          *fnNode, [](CGNode & node) -> auto { return node.getCallers(); },
          visitFn);
      std::cout << "Functions on call path to " << fn << ": " << count << "\n";
    }
  }
  return out;
}

template <SetOperation Op> FunctionSet SetOperationSelector<Op>::apply(const FunctionSetList& input) {
  static_assert(Op == SetOperation::UNION || Op == SetOperation::INTERSECTION ||
                Op == SetOperation::COMPLEMENT);

  if (input.size() != 2) {
    logError() << "Expected exactly two input sets, got " << input.size() << "instead.\n";
    return {};
  }

  FunctionSet inA = input[0];
  FunctionSet inB = input[1];

  FunctionSet out;

  if constexpr (Op == SetOperation::UNION) {
    out.insert(out.end(), inA.begin(), inA.end());
    out.insert(out.end(), inB.begin(), inB.end());
  } else if constexpr (Op == SetOperation::INTERSECTION) {
    for (auto &fA : inA) {
      if (std::find(inB.begin(), inB.end(), fA) != inB.end()) {
        out.push_back(fA);
      }
    }
  } else if constexpr (Op == SetOperation::COMPLEMENT) {
    for (auto &fA : inA) {
      if (std::find(inB.begin(), inB.end(), fA) == inB.end()) {
        out.push_back(fA);
      }
    }
  }
  return out;
}

class SelectorRunner {
  CallGraph &cg;

public:
  SelectorRunner(CallGraph &cg) : cg(cg) {}

  FunctionSet run(Selector &selector);
};

#endif // CAPI_SELECTOR_H
