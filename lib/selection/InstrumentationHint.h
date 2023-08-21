//
// Created by sebastian on 11.08.23.
//

#ifndef CAPI_INSTRUMENTATIONHINT_H
#define CAPI_INSTRUMENTATIONHINT_H

#include <string>
#include <vector>

namespace capi {

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

struct InstrumentationHint {
  InstrumentationType type;
  std::string selRefName;
};

using InstrumentationHints = std::vector<InstrumentationHint>;

struct InstrumentationHintCollector {
  virtual void addHint(InstrumentationHint hint) = 0;
};

}

#endif // CAPI_INSTRUMENTATIONHINT_H
