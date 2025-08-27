//
// Created by sebastian on 12.08.25.
//

// (1) Check if init/finalize instrumentation is inserted correctly with static instrumentation.
// Note: Using dummy home, since mpic++ fails within lit without it
// RUN: env HOME=/tmp %capi_cc --capi-interface=nesmik --verbose mpic++ -O2  -S -emit-llvm %s -o - | FileCheck %s

// (2) Check if init/finalize instrumentation is inserted correctly with XRay custom events.
// Note: Using dummy home, since mpic++ fails within lit without it
// RUN: env HOME=/tmp %capi_cc --capi-interface=nesmik --verbose mpic++ -mllvm -emit-xray-events=true -O2  -S -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-XRAY %s


#define XRAY_NEVER_INSTRUMENT __attribute__((xray_never_instrument))

#define NOINLINE __attribute__((noinline))

#include <mpi.h>
#include <cstdio>

NOINLINE void bar() {
  printf("bar called\n");
}


NOINLINE void foo(int n) {
  for (int i = 0; i < n; i++) {
    bar();
  }
}

// Checks for static instrumentation:
// CHECK: define dso_local {{.*}} i32 @main
// CHECK: call i32 @MPI_Init{{.*}}
// CHECK-NEXT: call void @dyncapi_nesmik_init{{.*}}
// CHECK: {{.*}}call {{.*}}void @_Z3fooi{{.*}}
// CHECK: call void @dyncapi_nesmik_finalize{{.*}}
// CHECK-NEXT: call i32 @MPI_Finalize{{.*}}
//
// Checks for XRay instrumentation:
// CHECK-XRAY: define dso_local {{.*}} i32 @main
// CHECK-XRAY: call i32 @MPI_Init{{.*}}
// CHECK-XRAY-NEXT: call void @llvm.xray.customevent(ptr @init.str, i64 13)
// CHECK-XRAY: {{.*}}call {{.*}}void @_Z3fooi{{.*}}
// CHECK-XRAY: call void @llvm.xray.customevent(ptr @finalize.str, i64 17)
// CHECK-XRAY-NEXT: call i32 @MPI_Finalize{{.*}}
int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  foo(3);

  MPI_Finalize();
  return 0;
}
