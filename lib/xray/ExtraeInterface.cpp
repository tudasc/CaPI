//
// Created by sebastian on 30.08.22.
//

#include "XRayRuntime.h"

#include <unordered_map>
#include <ranges>

#include "../Utils.h"
#include "SymbolRetriever.h"

extern "C" {
void Extrae_init() __attribute__((weak));
int Extrae_is_initialized() __attribute__((weak));
void Extrae_event(unsigned type, long long value) __attribute__((weak));
void Extrae_eventandcounters(unsigned type, long long value) __attribute__((weak));
void Extrae_define_event_type(unsigned *type, char *type_description, int *nvalues,
                              long long *values, char **values_description) __attribute__((weak));
}

namespace {

unsigned int ExtraeXRayEvt = 31169;

unsigned int ExtraeExit = 0;

bool initialized{false};

inline void handle_extrae_region_enter(int id) {
  Extrae_eventandcounters(ExtraeXRayEvt, id);
}

inline void handle_extrae_region_exit(int id) {
  Extrae_eventandcounters(ExtraeXRayEvt, ExtraeExit);
}

}

namespace capi {

void handleXRayEvent(int32_t id, XRayEntryType type) XRAY_NEVER_INSTRUMENT {

  if (!initialized) {
    static bool failedBefore{false};
    if (!failedBefore) {
      logError() << "Extrae interface has not been initialized.\n";
      failedBefore = true;
    }
    return;
  }

  switch (type) {
  case XRayEntryType::ENTRY:
    handle_extrae_region_enter(id);
    break;
  case XRayEntryType::TAIL:
  case XRayEntryType::EXIT:
    handle_extrae_region_exit(id);
    break;
  default:
    logError() << "Unhandled XRay event type.\n";
    break;
  }
}

void registerExtraeEvents(const XRayFunctionMap& xrayMap) {
  int nvalues = xrayMap.size();
  std::vector<long long> idList;
  idList.reserve(nvalues);
  std::vector<char*> descriptions;
  descriptions.reserve(nvalues);
  for (auto&& [id, info] : xrayMap) {
    idList.push_back(id);
    // Ugly but necessary, if we want to avoid copying all function names
    descriptions.push_back(const_cast<char*>(info.name.c_str()));
  }

  Extrae_define_event_type (&ExtraeXRayEvt, "XRay Event", &nvalues, &idList[0], &descriptions[0]);
}

void postXRayInit(const XRayFunctionMap& xrayMap) {
  if (!Extrae_init) {
    logError() << "Extrae API not available. XRay events will not be traced.\n";
    return;
  }
  if (!Extrae_is_initialized()) {
    Extrae_init();
    if (!Extrae_is_initialized()) {
      logError()
          << "Failed to initialize Extrae. XRay events will not be traced.\n";
      return;
    }
  }
  registerExtraeEvents(xrayMap);
  initialized = true;
  logInfo() << "XRAY initialization and Extrae event registration done.\n";
}

}
