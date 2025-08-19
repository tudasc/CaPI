//
// Created by sebastian on 11.08.23.
//

#ifndef CAPI_INSTRUMENTATIONACTION_H
#define CAPI_INSTRUMENTATIONACTION_H

#include <string>
#include <vector>
#include <map>

namespace capi {

using InvocationRange = std::pair<unsigned, unsigned>;
using Invocations = std::vector<InvocationRange>;
using MappedInvocations = std::map<std::string, Invocations>;

enum InstrumentationType {
  NONE = 0,
  ALWAYS_INSTRUMENT = 1,
  SCOPE_TRIGGER = 2,
  BEGIN_TRIGGER = 4,
  END_TRIGGER = 8,

};

inline bool isInstrumented(int flags) {
  return flags & 0b1111;
}

inline bool isAlwaysInstrumented(int flags) {
  return flags & ALWAYS_INSTRUMENT;
}

inline bool isScopeTrigger(int flags) {
  return flags & SCOPE_TRIGGER;
}

inline bool isBeginTrigger(int flags) {
  return flags & BEGIN_TRIGGER;
}

inline bool isEndTrigger(int flags) {
  return flags & END_TRIGGER;
}

struct InstrumentationAction {
  InstrumentationType type;
  std::string selRefName;
  MappedInvocations activeInvocations;
};

using InstrumentationActions = std::vector<InstrumentationAction>;

struct InstrumentationActionCollector {
  virtual void addAction(InstrumentationAction action) = 0;
};

}

#endif  // CAPI_INSTRUMENTATIONACTION_H
