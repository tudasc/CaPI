// clang-format off
//
// CG:
// main -> a -> b -> c -> MPI_ISend
//  |       \
//  f        d -> e -> MPI_Waitall
//
// Note: This code doesn't do anything useful and is not meant to be run.
//
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; %capi -i 'common_caller(by_name("@_Z1cPf", %%%%), by_name("@_Z1ev", %%%%))' -o %s.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s.filt | c++filt | sort | %filecheck %s
//
// clang-format on

#include <mpi.h>

void f() {
}

void e() {
  MPI_Waitall(1, nullptr, nullptr);
}

void d() {
  e();
}

void c(float* buf) {
  MPI_Isend(buf, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, nullptr);
}

void b(float* buf) {
  c(buf);
}

void a(int x) {
  float val = 0;
  b(&val);
  d();
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  f();
  a(argc);
  MPI_Finalize();
  return 0;
}

// CHECK: a
// CHECK: b
// CHECK: c
// CHECK: d
// CHECK: e
// CHECK-NOT: f
// CHECK-NOT: main

