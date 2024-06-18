// clang-format off
//
// This case has two distinct relevant calling contexts and contains a cycle.
// See 04_common_caller.svg for the CG (added edges: a3 -> a1, b4 -> b2)
//
// Note: This code doesn't do anything useful and is not meant to be run.
//
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; timeout 5s %capi -i 'common_caller(by_name("c1", %%%%), by_name("c2", %%%%))  ' -o %s.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s.filt | c++filt | sort | %filecheck %s
//
// RUN: infile="%s"; %capi -i 'common_caller_distinct(by_name("c1", %%%%), by_name("c2", %%%%))' -o %s.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s.filt | c++filt | sort | %filecheck -check-prefix=DISTINCT %s
//
// clang-format on


void a1();
void b2();

extern void MPI_1();
extern void MPI_2();
void c2() {MPI_2();}
void c1() {MPI_1();}
void b5() {c2();};
void b4() {c1(); b2();};
void b3() {b5();}
void b2() {b4();}
void b1() {b2(); b3();}
void a3() {c2(); a1();}
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
// CHECK: main
// CHECK-NOT: MPI_1
// CHECK-NOT: MPI_2


// DISTINCT: a1
// DISTINCT: a2
// DISTINCT: a3
// DISTINCT: b1
// DISTINCT: b2
// DISTINCT: b3
// DISTINCT: b4
// DISTINCT: b5
// DISTINCT: c1
// DISTINCT: c2
// DISTINCT-NOT: main
// DISTINCT-NOT: MPI_1
// DISTINCT-NOT: MPI_2
