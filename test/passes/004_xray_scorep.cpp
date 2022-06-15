//
// Created by sebastian on 21.03.22.
//

// RUN: scorep-config --mpp=none --adapter-init > scorep_init.c
// RUN: %clang_cc -c scorep_init.c -o scorep_init.o
// RUN: %clang_cxx -c -v -g -fxray-instrument -fxray-instruction-threshold=1 %s -o %s.o
// RUN: %clang_cxx -fuse-ld=lld -v -g -fxray-instrument -L `scorep-config --prefix`/lib -Wl,-start-group -Wl,--whole-archive -L %lib_dir/xray -l %capi_xray_scorep_lib -Wl,--no-whole-archive `scorep-config --libs --mpp=none --compiler` -Wl,-end-group  scorep_init.o %s.o -o %s.exe

// No filtering
// RUN: rm -rf scorep-test-profile*
// RUN: SCOREP_EXPERIMENT_DIRECTORY=scorep-test-profile SCOREP_ENABLE_PROFILING=true CAPI_EXE=%s.exe XRAY_OPTIONS="patch_premain=false verbosity=1" %s.exe
// RUN: scorep-score -r -s name scorep-test-profile/profile.cubex | FileCheck %s
// RUN: rm -r scorep-test-profile/

// Filtering
// RUN: SCOREP_EXPERIMENT_DIRECTORY=scorep-test-profile SCOREP_ENABLE_PROFILING=true CAPI_EXE=%s.exe CAPI_FILTERING_FILE="003_xray.filt" XRAY_OPTIONS="patch_premain=false verbosity=1" %s.exe
// RUN: scorep-score -r -s name scorep-test-profile/profile.cubex | FileCheck -check-prefix=CHECK-FILTERED %s
// RUN: rm -r scorep-test-profile/ %s.o %s.exe scorep_init.c scorep_init.o


#define XRAY_NEVER_INSTRUMENT __attribute__((xray_never_instrument))

#include <cstdio>

void foo(int n) {
  if (--n > 0)
    foo(n);
}

// CHECK: _Z3fooi
// CHECK: main

// CHECK-FILTERED: _Z3fooi
// CHECK-FILTERED-NOT: main

int main(int argc, char** argv) {
  foo(3);
  printf("Done");
}

