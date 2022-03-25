//
// Created by sebastian on 22.03.22.
//
// RUN: scorep-config --mpp=none --adapter-init > scorep_init.c
// RUN: clang -c scorep_init.c -o scorep_init.o
// RUN: clang++ -fxray-instrument -fxray-instruction-threshold=1 -fPIC -shared testlib.c -o libtestlib.so
// RUN: clang++ -c -v -g -fxray-instrument -fxray-instruction-threshold=1 shared.cpp -o shared.o
// RUN: clang++ -v -g -fxray-instrument -L `scorep-config --prefix`/lib -Wl,-start-group -Wl,--whole-archive -L %lib_dir/xray -l %dyncapi_scorep_lib -Wl,--no-whole-archive `scorep-config --libs --mpp=none --compiler` -Wl,-end-group -L./ -Wl,-rpath,./ -ltestlib scorep_init.o shared.o -o shared

// No filtering
// RUN: SCOREP_EXPERIMENT_DIRECTORY=scorep-test-profile SCOREP_ENABLE_PROFILING=true CAPI_EXE=basic XRAY_OPTIONS="patch_premain=false verbosity=1" ./shared
// RUN: scorep-score -r -s name scorep-test-profile/profile.cubex | FileCheck %s
// RUN: rm -r scorep-test-profile/

// Filtering
// RUN: SCOREP_EXPERIMENT_DIRECTORY=scorep-test-profile SCOREP_ENABLE_PROFILING=true CAPI_EXE=basic CAPI_FILTERING_FILE="basic.filt" XRAY_OPTIONS="patch_premain=false verbosity=1" ./shared
// RUN: scorep-score -r -s name scorep-test-profile/profile.cubex | FileCheck -check-prefix=CHECK-FILTERED %s
// RUN: rm -r scorep-test-profile/ basic.o basic scorep_init.c scorep_init.o


#define XRAY_NEVER_INSTRUMENT __attribute__((xray_never_instrument))

int lib_fn();

void foo(int n) {
  if (--n > 0)
    foo(n);
}


// CHECK: _Z3fooi
// CHECK: _Z6lib_fnv
// CHECK: main

// CHECK-FILTERED: _Z3fooi
// CHECK: _Z6lib_fnv
// CHECK-FILTERED-NOT: main

int main(int argc, char** argv) {
  int n = lib_fn();
  foo(n);
}
