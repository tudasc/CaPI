// clang-format off
//
// This case has two distinct relevant common ancestors.
// See 06_common_caller_unclear.svg for the CG.
//
// Note: This code doesn't do anything useful and is not meant to be run.
//
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; timeout 10s %capi -i 'common_caller(by_name("b", %%%%), by_name("d", %%%%))  ' -o %s.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s.filt | c++filt | sort | %filecheck %s
//
// RUN: infile="%s"; timeout 10s %capi -i 'common_caller(1, by_name("b", %%%%), by_name("d", %%%%))  ' -o %s_dist1.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_dist1.filt | c++filt | sort | %filecheck -check-prefix=MAX-DIST1 %s
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

// CHECK: a
// CHECK: b
// CHECK: c
// CHECK: d
// CHECK: e
// CHECK: main
// CHECK-NOT: MPI_1
// CHECK-NOT: MPI_2

// MAX-DIST1: a
// MAX-DIST1: b
// MAX-DIST1: c
// MAX-DIST1: d
// MAX-DIST1: e
// MAX-DIST1: main
// MAX-DIST1-NOT: MPI_1
// MAX-DIST1-NOT: MPI_2

