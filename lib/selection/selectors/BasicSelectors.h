//
// Created by sebastian on 15.03.22.
//

#ifndef CAPI_BASICSELECTORS_H
#define CAPI_BASICSELECTORS_H

#include <iostream>
#include <regex>

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

  std::string getName() {
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

}

#endif //CAPI_BASICSELECTORS_H
