// clang-format off
//
// Note: This code doesn't do anything useful and is not meant to be run.
//
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; timeout 10s %capi -i 'callContext2(0, byName("_Z2t1v", %%%%), byName("_Z2t2v", %%%%))' -o %s_dist0.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_dist0.filt | c++filt | sort | %filecheck -check-prefix=MAX-DIST0 %s
//
// RUN: infile="%s"; timeout 10s %capi -i 'callContext2(1, byName("_Z2t1v", %%%%), byName("_Z2t2v", %%%%))' -o %s_dist1.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_dist1.filt | c++filt | sort | %filecheck -check-prefix=MAX-DIST1 %s
//
// RUN: infile="%s"; timeout 10s %capi -i 'callContext2(2, byName("_Z2t1v", %%%%), byName("_Z2t2v", %%%%))' -o %s_dist2.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_dist2.filt | c++filt | sort | %filecheck -check-prefix=MAX-DIST2 %s
//
// RUN: infile="%s"; timeout 10s %capi -i 'callContext2(3, byName("_Z2t1v", %%%%), byName("_Z2t2v", %%%%))' -o %s_dist3.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_dist3.filt | c++filt | sort | %filecheck -check-prefix=MAX-DIST3 %s
//
// clang-format on


extern void MPI_1();
extern void MPI_2();
void t1() {MPI_1();};
void t2() {MPI_2();}
void d() {t1(); t2();}
void c() {d();}
void b() {t1(); c();}
void a() {t1(); b();}

int main(int argc, char** argv) {
  a();
  return 0;
}

// MAX-DIST0-NOT: a
// MAX-DIST0-NOT: b
// MAX-DIST0-NOT: c
// MAX-DIST0: d
// MAX-DIST0-NOT: main
// MAX-DIST0-NOT: MPI_1
// MAX-DIST0-NOT: MPI_2
// MAX-DIST0: t1
// MAX-DIST0: t2

// MAX-DIST1-NOT: a
// MAX-DIST1: b
// MAX-DIST1: c
// MAX-DIST1: d
// MAX-DIST1-NOT: main
// MAX-DIST1-NOT: MPI_1
// MAX-DIST1-NOT: MPI_2
// MAX-DIST1: t1
// MAX-DIST1: t2

// MAX-DIST2: a
// MAX-DIST2: b
// MAX-DIST2: c
// MAX-DIST2: d
// MAX-DIST2-NOT: main
// MAX-DIST2-NOT: MPI_1
// MAX-DIST2-NOT: MPI_2
// MAX-DIST2: t1
// MAX-DIST2: t2

// MAX-DIST3: a
// MAX-DIST3: b
// MAX-DIST3: c
// MAX-DIST3: d
// MAX-DIST3-NOT: main
// MAX-DIST3-NOT: MPI_1
// MAX-DIST3-NOT: MPI_2
// MAX-DIST3: t1
// MAX-DIST3: t2