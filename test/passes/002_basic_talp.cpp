//
// Created by sebastian on 07.06.22.
//

#include <mpi.h>

// basic.filt: main and foo are instrumented, bar is not.

// (1) Check if region instrumentation is inserted correctly.
// Note: Using dummy home, since mpic++ fails within lit without it
// RUN: env HOME=/tmp %capi_cc --capi-interface=talp_static --verbose mpic++ -O2  -mllvm -inst-filter="$(dirname %s)/001_basic.filt" -S -emit-llvm %s -o - | FileCheck %s

// (2) Run with TALP, check if region statistics are emitted.
// RUN: env HOME=/tmp %capi_cc --capi-interface=talp_static --verbose mpic++ -O2  -mllvm -inst-filter="$(dirname %s)/001_basic.filt" %talp_args %s -o %s.o
// RUN: env HOME=/tmp DLB_ARGS="--talp --talp-summary=pop-metrics" mpirun -n 2 %s.o 2>&1 | FileCheck %s -check-prefix=CHECK-TALP

// CHECK-TALP: DLB{{.*}} ### Name: _Z3fooi
// CHECK-TALP-NOT: DLB{{.*}} ### Name: _Z3bari

// CHECK: @0 = private unnamed_addr constant [8 x i8] c"_Z3fooi\00", align 1

// CHECK: define dso_local {{.*}} i32 @_Z3bari
// CHECK-NOT:  {{.*}}@DLB_MonitoringRegion{{.*}}
// CHECK: }
int __attribute__((noinline)) bar(int x) {
  return x * 2;
}

// CHECK: define dso_local {{.*}} i32 @_Z3fooi
// CHECK:  [[R0:%.*]] = call ptr @DLB_MonitoringRegionRegister(ptr @0)
// CHECK-NEXT:  [[R1:%.*]] = call i32 @DLB_MonitoringRegionStart(ptr [[R0]])
// CHECK-NEXT:  [[CALL:%.*]] = tail call noundef i32 @_Z3bari
// CHECK-NEXT:  [[ADD:%.*]] = add nsw i32 [[CALL]], 5
// CHECK-NEXT:  [[R2:%.*]] = call i32 @DLB_MonitoringRegionStop(ptr [[R0]])
// CHECK-NEXT:  ret i32 [[ADD]]
// CHECK-NEXT: }
int __attribute__((noinline))  foo(int x) {
  return bar(x) + 5;
}

// CHECK: define dso_local {{.*}} i32 @main
// CHECK-NOT:  {{.*}}@DLB_MonitoringRegion{{.*}}
// CHECK: {{.*}}call noundef i32 @_Z3fooi{{.*}}
// CHECK:  ret i32 0
// CHECK-NEXT }
int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int x = foo(argc);
  int sum = 0;
  MPI_Allreduce(&x, &sum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

  MPI_Finalize();
  return 0;
}
