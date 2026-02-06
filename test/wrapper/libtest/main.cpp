//
// Created by sebastian on 28.01.26.
//


#include <iostream>
#include <dlfcn.h>

#define XRAY_NEVER_INSTRUMENT __attribute__((xray_never_instrument))


extern "C" void __cyg_profile_func_enter(void* addr, void* callsite) XRAY_NEVER_INSTRUMENT {
  std::cout << "<Entering " << addr << ">\n";
}

extern "C" void __cyg_profile_func_exit(void* addr, void* callsite) XRAY_NEVER_INSTRUMENT {
  std::cout << "<Exiting " << addr << ">\n";
}

extern "C" const char* greeting();

int main(int argc, char** argv) {
  std::cout << "This is a test application!\n";
  std::cout << "Shared library says: " << greeting() << "\n";
  #ifdef DLOPEN_LIB
  void* handle = dlopen("shared2.so", RTLD_NOW);
  if (!handle) {
    std::cout << "Failed to load shared library!\n";
    return 1;
  }
  using FnType = const char* (*)();
  FnType greeting2 = (FnType) dlsym(handle, "greeting2");
  std::cout << "Dynamically loaded library says: " << greeting2() << "\n";
  #endif
  return 0;
}

