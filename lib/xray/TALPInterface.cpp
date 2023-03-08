//
// Created by sebastian on 30.08.22.
//

#include "XRayRuntime.h"

#include <unordered_map>

#include "../Utils.h"
#include "SymbolRetriever.h"

#include "dlb_talp.h"
#include "dlb_errors.h"


#define TALP_NAME_MAX 128

namespace {

using namespace capi;

inline bool isTALPActive() {
  return DLB_MonitoringRegionGetMPIRegion() != nullptr;
}

struct RegionInfo {
  dlb_monitor_t* monitor{nullptr};
  bool ignore{false};

  bool isInitialized() const {
    return monitor != nullptr;
  }

  const char* name() {
    if (monitor)
      return monitor->name;
    return "";
  }
};

struct TalpData {
  SymbolTable symbolTable;
  std::unordered_map<uintptr_t, RegionInfo> regionMap;
  bool talpActive{false};
  bool talpWasActive{false};

  bool updateTALPState() {
    if (isTALPActive()) {
      talpWasActive = true;
      talpActive = true;
    } else {
      talpActive = false;
    }
    return talpActive;
  }

  bool isTALPFinished() const {
    return !talpActive && talpWasActive;
  }
};

// Note: This is behind a pointer to prevent initialization order problems;
TalpData* talpData{nullptr};
bool initialized{false};

inline void handle_talp_region_enter(RegionInfo &region) {
  if (!region.isInitialized()) {
    logError() << "Trying to enter unregistered region - skipping.\n";
    return;
  }
  auto state = DLB_MonitoringRegionStart(region.monitor);
  if (state != DLB_SUCCESS) {
    if (!talpData->updateTALPState()) {
      logError() << "Encountered event after TALP has been finalized. Stopping monitoring.\n";
      return;
    }
    logError() << "Failed to enter TALP region: " << region.name() << "\n";
  }
}

inline void handle_talp_region_exit(RegionInfo &region) {
  if (!region.isInitialized()) {
    logError() << "Trying to exit unregistered region - skipping.\n";
    return;
  }
  auto state = DLB_MonitoringRegionStop(region.monitor);
  if (state != DLB_SUCCESS) {
    if (!talpData->updateTALPState()) {
      logError() << "Encountered event after TALP has been finalized. Stopping monitoring.\n";
      return;
    }
    logError() << "Failed to exit TALP region: " << region.name() << "\n";
  }
}

}

namespace capi {

void handleXRayEvent(int32_t id, XRayEntryType type) XRAY_NEVER_INSTRUMENT {

  // TODO: Optimize region lookup at exit by maintaining thread-local stack of active regions?

  if (!initialized) {
    logError() << "TALP region handler has not been initialized.\n";
    return;
  }

  TalpData& data = *talpData;

  // Skip if TALP is finished
  if (data.isTALPFinished()) {
    return;
  }

  auto& region = data.regionMap[id];
  if (region.ignore) {
    // Region could not be initialized and is thus ignored in future calls.
    // TODO: Unpatch?
    return;
  }
  if (!region.isInitialized()) {
    // Registering region
    uintptr_t addr = __xray_function_address(id);
    if (!addr) {
      logError() << "Unable to map function ID " << id << " to a symbol.\n";
      region.ignore = true;
      return;
    }
    auto it = data.symbolTable.find(addr);
    if (it == data.symbolTable.end()) {
      logError() << "Unable to detect symbol name at address " << std::hex << addr << std::dec  << ". Skipping region.\n";
      logError() << "Symbol table entries: " << data.symbolTable.size() << "\n";
      region.ignore = true;
      return;
    }
    auto& name = it->second;
    if (name.size() >= TALP_NAME_MAX) {
      logError() << "Function name too long for TALP - skipping:\n";
      logError() << name << "\n";
      region.ignore = true;
      return;
    }
    if (!data.talpActive) {
      if (!data.updateTALPState()) {
        return;
      }
    }
    region.monitor = DLB_MonitoringRegionRegister(name.c_str());
    if (!region.monitor) {
      logError() << "Registering TALP region failed: " << name << "\n";
      region.ignore = true;
      return;
    }
  }


  switch (type) {
  case XRayEntryType::ENTRY:
    handle_talp_region_enter(region);
    break;
  case XRayEntryType::TAIL:
  case XRayEntryType::EXIT:
    handle_talp_region_exit(region);
    break;
  default:
    logError() << "Unhandled XRay event type.\n";
    break;
  }
}

void postXRayInit(const SymbolTable& symTable) {
    talpData = new TalpData{symTable};
    initialized = true;
    logInfo() << "XRAY has been initialized, data passed to TALP handler.\n";
}

}
