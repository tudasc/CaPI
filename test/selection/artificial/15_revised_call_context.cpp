// clang-format off
//
// Note: This code doesn't do anything useful and is not meant to be run.
//
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; timeout 10s %capi -i 'common_caller(0, by_name("x", %%%%), by_name("y", %%%%))' -o %s_dist0.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_dist0.filt | c++filt | sort | %filecheck -check-prefix=MAX-DIST0 %s
//
// RUN: infile="%s"; timeout 10s %capi -i 'common_caller(1, by_name("x", %%%%), by_name("y", %%%%))' -o %s_dist1.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_dist1.filt | c++filt | sort | %filecheck -check-prefix=MAX-DIST1 %s
//
// RUN: infile="%s"; timeout 10s %capi -i 'common_caller(2, by_name("x", %%%%), by_name("y", %%%%))' -o %s_dist2.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_dist2.filt | c++filt | sort | %filecheck -check-prefix=MAX-DIST2 %s
//
// clang-format on


void x();
void y();
void b() {x(); y();}
void c() {x(); y();}
void d() {b(); y();}
void a() {b(); d();}
void f() {a();}
void e() {x(); f();}

// MAX-DIST0-NOT: a
// MAX-DIST0: b
// MAX-DIST0: c
// MAX-DIST0-NOT: d
// MAX-DIST0-NOT: e
// MAX-DIST0-NOT: f
// MAX-DIST0: x
// MAX-DIST0: y

// MAX-DIST1: a
// MAX-DIST1: b
// MAX-DIST1: c
// MAX-DIST1: d
// MAX-DIST1: e
// MAX-DIST1: f
// MAX-DIST1: x
// MAX-DIST1: y

// MAX-DIST2: a
// MAX-DIST2: b
// MAX-DIST2: c
// MAX-DIST2: d
// MAX-DIST2: e
// MAX-DIST2: f
// MAX-DIST2: x
// MAX-DIST2: y