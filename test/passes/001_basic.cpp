//
// Created by sebastian on 07.06.22.
//

// TODO: Currenlty unsupported due to: unknown cgscc pass 'post-inline-ee-instrument'
// XFAIL: *

// basic.filt: main and foo are instrumented, bar is not.

// (1) Check if the attributes are set correctly:
// RUN: env HOME=/tmp %capi_cc --capi-interface=gnu_static --verbose clang++ -O2 -mllvm -inst-filter="$(dirname %s)/001_basic.filt" -S -emit-llvm %s -o - | FileCheck %s

// (2) Check if the instrumentation is applied:
// RUN: %cpp-to-llvm %s | %capi-opt -inst-api="gnu" -inst-filter="$(dirname %s)/001_basic.filt" -S | %opt -passes="inline,post-inline-ee-instrument" -S | FileCheck %s -check-prefix=CHECK-INST


// CHECK: define dso_local {{.*}} i32 @_Z3bari(i32 noundef %x) local_unnamed_addr #[[#BARATTR:]]
// CHECK: define dso_local {{.*}} i32 @_Z3fooi(i32 noundef %x) local_unnamed_addr #[[#FOOMAINATTR:]]
// CHECK: define dso_local {{.*}} i32 @main(i32 noundef %argc, ptr nocapture noundef readnone %argv) local_unnamed_addr #[[#FOOMAINATTR]]
// CHECK-NOT: attributes #[[#BARATTR]] = { {{.*}} "instrument-function-entry-inlined"="__cyg_profile_func_enter" "instrument-function-exit-inlined"="__cyg_profile_func_exit"
// CHECK: attributes #[[#FOOMAINATTR]] = { {{.*}} "instrument-function-entry-inlined"="__cyg_profile_func_enter" "instrument-function-exit-inlined"="__cyg_profile_func_exit"


// CHECK-INST: define dso_local  {{.*}} i32 @_Z3bari
// CHECK-INST-NOT: call void @__cyg_profile_func_enter
// CHECK-INST-NOT: call void @__cyg_profile_func_exit
int bar(int x) {
  return x * 2;
}

// CHECK-INST: define dso_local {{.*}} i32 @_Z3fooi
// CHECK-INST: call void @__cyg_profile_func_enter
// CHECK-INST: call void @__cyg_profile_func_exit
int foo(int x) {
  return bar(x) + 5;
}

// CHECK-INST: define dso_local {{.*}} i32 @main
// CHECK-INST: call void @__cyg_profile_func_enter
// CHECK-INST: call void @__cyg_profile_func_exit
int main(int argc, char** argv) {
  return foo(argc);
}
