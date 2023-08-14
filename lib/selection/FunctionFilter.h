//
// Created by sebastian on 11.02.22.
//

#ifndef CAPI_FUNCTIONFILTER_H
#define CAPI_FUNCTIONFILTER_H

#include <string>
#include <vector>
#include <unordered_map>

#include "InstrumentationHint.h"

namespace capi {

class FunctionFilter
{
  std::unordered_map<std::string, int> instFlags;

public:
  FunctionFilter();

  size_t size() const {
    return instFlags.size();
  }

  bool accepts(const std::string& f) const;

  int getFlags(const std::string& f) const;

  void addIncludedFunction(const std::string &f, int flags);

  void addIncludedFunction(const std::string &f) {
    addIncludedFunction(f, InstrumentationType::ALWAYS_INSTRUMENT);
  }

  void removeIncludedFunction(const std::string &f);

  decltype(instFlags)::iterator begin()
  {
    return instFlags.begin();
  }

  decltype(instFlags)::iterator end()
  {
    return instFlags.end();
  }
};

bool readScorePFilterFile(FunctionFilter &filter, const std::string &filename);

bool writeScorePFilterFile(FunctionFilter &filter, const std::string &filename);

bool writeSimpleFilterFile(FunctionFilter &filter, const std::string &filename);

bool readJSONFilterFile(FunctionFilter &filter, const std::string &filename);

bool writeJSONFilterFile(FunctionFilter &filter, const std::string& filename);


}

#endif // CAPI_FUNCTIONFILTER_H
