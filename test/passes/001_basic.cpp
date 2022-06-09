//
// Created by sebastian on 07.06.22.
//

// RUN: %capi_cc -O2 -mllvm -inst-api="talp" -mllvm -inst-filter="$(dirname %s)/001_basic.filt" -S -emit-llvm %s -o - | FileCheck %s
// UN: %cpp-to-llvm %s | %capi-opt -inst-api="talp" -inst-filter="$(dirname %s)/001_basic.filt" -S 2>&1 | FileCheck %s


// CHECK: define dso_local i32 @_Z3fooi(i32 %0) local_unnamed_addr #0 {
// CHECK-NEXT: %2 = call %dlb_monitor_t* @DLB_MonitoringRegionRegister(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @0, i32 0, i32 0))
// CHECK-NEXT: %3 = call i32 @DLB_MonitoringRegionStart(%dlb_monitor_t* %2)
// CHECK-NEXT: %4 = add nsw i32 %0, 5
// CHECK-NEXT: %5 = call i32 @DLB_MonitoringRegionStop(%dlb_monitor_t* %2)
// CHECK-NEXt: ret i32 %4
//}

int foo(int x) {
  return x + 5;
}
// CHECK: define dso_local i32 @main(i32 %0, i8** nocapture readnone %1) local_unnamed_addr #0 {
// CHECK-NEXT:  %3 = call %dlb_monitor_t* @DLB_MonitoringRegionRegister(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @1, i32 0, i32 0))
// CHECK-NEXT:  %4 = call i32 @DLB_MonitoringRegionStart(%dlb_monitor_t* %3)
// CHECK-NEXT:  %5 = add nsw i32 %0, 5
// CHECK-NEXT:  %6 = call i32 @DLB_MonitoringRegionStop(%dlb_monitor_t* %3)
// CHECK-NEXT:  ret i32 %5
//}
int main(int argc, char** argv) {
  return foo(argc);
}
