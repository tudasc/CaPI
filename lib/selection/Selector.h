//
// Created by sebastian on 28.10.21.
//

#ifndef CAPI_SELECTOR_H
#define CAPI_SELECTOR_H

#include <iostream>

#include "../Utils.h"
#include "CallGraph.h"
#include "MetaCGReader.h"

namespace capi {

using FunctionSet = std::vector<std::string>;

using FunctionSetList = std::vector<FunctionSet>;

class Selector
{
public:

  virtual ~Selector() = default;

  virtual void init(CallGraph &cg)
  {}

  virtual FunctionSet apply(const FunctionSetList &) = 0;

  virtual std::string getName() = 0;
};

using SelectorPtr = std::unique_ptr<Selector>;


class FilterSelector : public Selector
{

protected:
  CallGraph *cg;

public:
  FilterSelector() = default;

  void init(CallGraph &cg) override
  {
    this->cg = &cg;
  }

  virtual bool accept(const std::string &) = 0;

  FunctionSet apply(const FunctionSetList &input) override
  {
    if (input.size() != 1) {
      logError() << "Expected exactly one input set, got " << input.size() << " instead.\n";
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

}

#endif // CAPI_SELECTOR_H
