// clang-format off
//
// This case has two distinct relevant calling contexts.
// See 06_call_context_unclear.svg for the CG.
//
// Note: This code doesn't do anything useful and is not meant to be run.
//
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; timeout 10s %capi -i 'callContext(byName("_Z1bv", %%%%), byName("_Z1dv", %%%%))  ' -o %s.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s.filt | c++filt | sort | %filecheck %s
//
// clang-format on


extern void MPI_1();
extern void MPI_2();
void b() {MPI_1();}
void a() {b();}
void d() {MPI_2();}
void c() {d();}
void e() {b();d();}

int main(int argc, char** argv) {
  a();
  e();
  c();
  return 0;
}

// CHECK-NOT: a
// CHECK: b
// CHECK-NOT: c
// CHECK: d
// CHECK: e
// CHECK-NOT: main
// CHECK-NOT: MPI_1
// CHECK-NOT: MPI_2
