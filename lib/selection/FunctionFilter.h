//
// Created by sebastian on 11.02.22.
//

#ifndef CAPI_FUNCTIONFILTER_H
#define CAPI_FUNCTIONFILTER_H

#include <string>
#include <vector>

namespace capi {

class FunctionFilter
{
  std::vector<std::string> includedFunctionsMangled;

public:
  FunctionFilter();

  void addIncludedFunction(const std::string &f);

  void removeIncludedFunction(const std::string &f);

  decltype(includedFunctionsMangled)::iterator begin()
  {
    return includedFunctionsMangled.begin();
  }

  decltype(includedFunctionsMangled)::iterator end()
  {
    return includedFunctionsMangled.end();
  }
};

bool readScorePFilterFile(FunctionFilter &filter, const std::string &filename);

bool writeScorePFilterFile(FunctionFilter &filter, const std::string &filename);

}

#endif // CAPI_FUNCTIONFILTER_H
