//
// Created by sebastian on 07.06.22.
//

#include <mpi.h>

// basic.filt: main and foo are instrumented, bar is not.

// (1) Check if region instrumentation is inserted correctly.
// RUN: %capi_mpicxx -O2 -mllvm -inst-api="talp" -mllvm -inst-filter="$(dirname %s)/basic.filt" -S -emit-llvm %s -o - | FileCheck %s

// (2) Run with TALP, check if region statistics are emitted.
// RUN: %capi_mpicxx -O2 %talp_args -mllvm -inst-api="talp" -mllvm -inst-filter="$(dirname %s)/basic.filt" %s -o %s.o
// RUN: DLB_ARGS="--talp --talp-summary=pop-metrics" mpirun -n 2 %s.o 2>&1 | FileCheck %s -check-prefix=CHECK-TALP

// CHECK-TALP: DLB{{.*}} ### Name: MPI Execution
// CHECK-TALP: DLB{{.*}} ### Name: _Z3fooi
// CHECK-TALP-NOT: DLB{{.*}} ### Name: _Z3bari

// CHECK: @0 = private unnamed_addr constant [8 x i8] c"_Z3fooi\00", align 1

// CHECK: define dso_local i32 @_Z3bari
// CHECK-NOT:  {{.*}}@DLB_MonitoringRegion{{.*}}
// CHECK: }
int __attribute__((noinline)) bar(int x) {
  return x * 2;
}

// CHECK: define dso_local i32 @_Z3fooi(i32 %x) local_unnamed_addr #4 {
// CHECK:  %0 = call %dlb_monitor_t* @DLB_MonitoringRegionRegister(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @0, i32 0, i32 0))
// CHECK-NEXT:  %1 = call i32 @DLB_MonitoringRegionStart(%dlb_monitor_t* %0)
// CHECK-NEXT:  %call = tail call i32 @_Z3bari(i32 %x
// CHECK-NEXT:  %add = add nsw i32 %call, 5
// CHECK-NEXT:  %2 = call i32 @DLB_MonitoringRegionStop(%dlb_monitor_t* %0)
// CHECK-NEXT:  ret i32 %add
// CHECK-NEXT: }
int __attribute__((noinline))  foo(int x) {
  return bar(x) + 5;
}

// CHECK: define dso_local i32 @main
// CHECK-NOT:  {{.*}}@DLB_MonitoringRegion{{.*}}
// CHECK: {{.*}}call i32 @_Z3fooi{{.*}}
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
