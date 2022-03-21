//
// Created by sebastian on 21.03.22.
//

// RUN: clang++ -c -v -g -fxray-instrument -fxray-instruction-threshold=1 basic.cpp -o basic.o
// RUN: clang++ -v -g -fxray-instrument -Wl,--whole-archive -L %lib_dir/xray -l %dyncapi_lib -Wl,--no-whole-archive basic.o -L %lib_dir/calltree_verifier -lcalltreeverifier_static -o basic
// RUN: CAPI_EXE=basic XRAY_OPTIONS="patch_premain=false verbosity=1" ./basic
// RUN: cat basic.capi.log | FileCheck %s
// : rm basic.capi.log basic.o basic

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
  std::cout << "foo\n";
  if (--n > 0)
    foo(n);
}

// CHECK: [ENTER] 0x{{[[:xdigit:]]+}} main
// CHECK: [ENTER] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK: [ENTER] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK: [ENTER] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK: [EXIT] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK: [EXIT] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK: [EXIT] 0x{{[[:xdigit:]]+}} _Z3fooi
// CHECK: [EXIT] 0x{{[[:xdigit:]]+}} main
int main(int argc, char** argv) {
  foo(3);
}

