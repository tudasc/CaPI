//
// Created by sebastian on 22.03.22.
//

#include "ScorePInit.h"
#include "../Utils.h"

#include <cstring>

#define XRAY_NEVER_INSTRUMENT __attribute__((xray_never_instrument))


namespace capi {

void initScoreP(SymbolTable symbols) XRAY_NEVER_INSTRUMENT {

  auto isEnabled = [](auto env) {
    auto envVal = std::getenv(env);
    return envVal && strcmp(envVal, "true") == 0;
  };

  bool initializeScoreP = isEnabled("SCOREP_ENABLE_PROFILING") || isEnabled("SCOREP_ENABLE_TRACING");
  if (!initializeScoreP) {
    return;
  }
  logInfo() << "Initializing Score-P\n";

  // Initializing ScoreP
  SCOREP_InitMeasurement();

  for (auto&& [addr, name] : symbols) {
    scorep_compiler_hash_put(addr, name.c_str(), name.c_str(), "", 0);
  }

}

}
