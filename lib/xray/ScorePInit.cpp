//
// Created by sebastian on 22.03.22.
//

#include "ScorePInit.h"
#include "support/Logging.h"

#include <cstring>
#include <cctype>
#include <algorithm>
#include <string_view>

#define XRAY_NEVER_INSTRUMENT __attribute__((xray_never_instrument))


namespace capi {

bool ichar_equals(char a, char b)
{
  return std::tolower(static_cast<unsigned char>(a)) ==
         std::tolower(static_cast<unsigned char>(b));
}
bool iequals(std::string_view a, std::string_view b)
{
  return std::equal(a.begin(), a.end(), b.begin(), b.end(), ichar_equals);
}

void initScoreP(const XRayFunctionMap& xrayMap) XRAY_NEVER_INSTRUMENT {

  auto isEnabled = [](auto env) {
    auto envVal = std::getenv(env);
    // Emulates parseBool in SCOREP_Config.c
    return envVal && (iequals(envVal, "true") || iequals(envVal, "on") || iequals(envVal, "yes") || iequals(envVal, "1"));
  };

  bool initializeScoreP = isEnabled("SCOREP_ENABLE_PROFILING") || isEnabled("SCOREP_ENABLE_TRACING");
  if (!initializeScoreP) {
    return;
  }
  logInfo() << "Initializing Score-P\n";

  // Initializing ScoreP
  SCOREP_InitMeasurement();

  for (auto&& [fid, info] : xrayMap) {
    scorep_compiler_hash_put(info.addr, info.name.c_str(), info.demangled.c_str(), "", 0);
  }

}

void finalizeScoreP() XRAY_NEVER_INSTRUMENT {
}

}
