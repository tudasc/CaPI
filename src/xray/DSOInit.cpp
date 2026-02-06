//
// Created by sebastian on 03.02.26.
//

#include <cstdint>

// TODO: This is a horrible hack. Change as soon as object registration callbacks are available in XRay.
struct XRaySledEntry {
  uint64_t Address;
  uint64_t Function;
  unsigned char Kind;
  unsigned char AlwaysInstrument;
  unsigned char Version;
  unsigned char Padding[13]; // Need 32 bytes
  uint64_t function() const {
    // The target address is relative to the location of the Function variable.
    return reinterpret_cast<uint64_t>(&Function) + Function;
  }
  uint64_t address() const {
    // The target address is relative to the location of the Address variable.
    return reinterpret_cast<uint64_t>(&Address) + Address;
  }
};

extern "C" {
extern const XRaySledEntry __start_xray_instr_map[] __attribute__((weak));
extern const XRaySledEntry __stop_xray_instr_map[] __attribute__((weak));
}

extern "C" void capi_register_dso(uint64_t addr);


__attribute__((constructor))
static void capi_dso_init() {
  if (!__start_xray_instr_map) {
    return;
  }
  const XRaySledEntry* firstSled = &__start_xray_instr_map[0];
  capi_register_dso(firstSled->function());
}