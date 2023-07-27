// clang-format off
//
// CG:
// main -> a -> b -> c -> MPI_ISend
//  |       \
//  f        D1::d
//           D2::d (overrides D1::d)
//            \
//             e -> MPI_Waitall
//
// e overrides d
//
// Note: This code doesn't do anything useful and is not meant to be run.
//
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; %capi -i 'callContext2(byName("c", %%%%), byName("e", %%%%))' -o %s.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s.filt | c++filt | sort | %filecheck %s
//
// clang-format on

#include <mpi.h>

void f() {
}

void e() {
  MPI_Waitall(1, nullptr, nullptr);
}

struct D {
  virtual void d() {
  }
};

struct D2: public D {
  void d() override {
    e();
  }
};

void c(float* buf) {
  MPI_Isend(buf, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, nullptr);
}

void b(float* buf) {
  c(buf);
}

void a(int x) {
  float val = 0;
  b(&val);
  D* d;
  if (x < 0)
    d = new D;
  else
    d = new D2;
  d->d();
}

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);
  f();
  a(argc);
  MPI_Finalize();
  return 0;
}

// CHECK-DAG: a
// CHECK-DAG: b
// CHECK-DAG: c
// CHECK-DAG: D2::d
// CHECK-DAG: e
// CHECK-NOT: f
// CHECK-NOT: main

