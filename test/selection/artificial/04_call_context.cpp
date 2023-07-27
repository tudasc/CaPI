// clang-format off
//
// This case has two distinct relevant calling contexts.
// See 04_call_context.svg for the CG.
//
// Note: This code doesn't do anything useful and is not meant to be run.
//
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; %capi -i 'callContext2(byName("@_Z2c1v", %%%%), byName("@_Z2c2v", %%%%))' -o %s.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s.filt | c++filt | sort | %filecheck %s
//
// clang-format on

extern void MPI_1();
extern void MPI_2();
void c2() {MPI_2();}
void c1() {MPI_1();}
void b5() {c2();};
void b4() {c1();};
void b3() {b5();}
void b2() {b4();}
void b1() {b2(); b3();}
void a3() {c2();}
void a2() {c1();}
void a1() {a2(); a3();}

int main(int argc, char** argv) {
  a1();
  b1();
  return 0;
}

// CHECK: a1
// CHECK: a2
// CHECK: a3
// CHECK: b1
// CHECK: b2
// CHECK: b3
// CHECK: b4
// CHECK: b5
// CHECK: c1
// CHECK: c2
// CHECK-NOT: main
// CHECK-NOT: MPI_1
// CHECK-NOT: MPI_2
