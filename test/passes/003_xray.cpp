//
// Created by sebastian on 21.03.22.
//

// RUN: %clang_cc -c -v -g -fxray-instrument -fxray-instruction-threshold=1 %s -o %s.o
// RUN: %clang_cxx -v -g -fxray-instrument -Wl,--whole-archive -L %lib_dir/xray -l %capi_xray_lib -Wl,--no-whole-archive %s.o -L %lib_dir/calltree_logger -lcalltreelogger_static -o %s.exe
// RUN: CAPI_EXE=$(basename %s.exe) XRAY_OPTIONS="patch_premain=false verbosity=1" %s.exe
// RUN: cat %s.exe.capi.log | FileCheck %s
// RUN: rm %s.exe.capi.log
// RUN: CAPI_EXE=$(basename %s.exe) CAPI_FILTERING_FILE="003_xray.filt" XRAY_OPTIONS="patch_premain=false verbosity=1" %s.exe
// RUN: cat %s.exe.capi.log | FileCheck -check-prefix=CHECK-FILTERED %s
// RUN: rm %s.exe.capi.log %s.o %s.exe

#include <iostream>

#define XRAY_NEVER_INSTRUMENT __attribute__((xray_never_instrument))


extern "C" {

//void __cyg_profile_func_enter(void *fn, void *callsite) XRAY_NEVER_INSTRUMENT {
//  std::cout << "Entered function: " << fn << "\n";
//}
//
//void __cyg_profile_func_exit(void *fn, void *callsite) XRAY_NEVER_INSTRUMENT {
//  std::cout << "Exit function: " << fn << "\n";
//}
}

void foo(int n) {
  if (--n > 0)
    foo(n);
}

// CHECK: [ENTER] 0x{{[[:xdigit:]]+}} main
// CHECK-NEXT: [ENTER] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK-NEXT: [ENTER] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK-NEXT: [ENTER] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK-NEXT: [EXIT] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK-NEXT: [EXIT] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK-NEXT: [EXIT] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK-NEXT: [EXIT] 0x{{[[:xdigit:]]+}} main

// CHECK-FILTERED-NOT: [ENTER] 0x{{[[:xdigit:]]+}} main
// CHECK-FILTERED: [ENTER] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK-FILTERED-NEXT: [ENTER] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK-FILTERED-NEXT: [ENTER] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK-FILTERED-NEXT: [EXIT] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK-FILTERED-NEXT: [EXIT] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK-FILTERED-NEXT: [EXIT] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK-FILTERED-NOT: [EXIT] 0x{{[[:xdigit:]]+}} main
int main(int argc, char** argv) {
  foo(3);
}

