// clang-format off
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; %capi -i 'by_name(".*", %%%%)' -o /dev/null --print-scc-stats ${infile%%.*}.ipcg | %filecheck %s -check-prefix=LOG
//
// RUN: infile="%s"; %capi -i 'on_call_path_to(by_name("a", %%%%))' -o %s_callers_a.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_callers_a.filt | c++filt | sort | %filecheck %s -check-prefix=CALLERS-A
//
// RUN: infile="%s"; %capi -i 'on_call_path_to(by_name("f", %%%%))' -o %s_callers_f.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_callers_f.filt | c++filt | sort | %filecheck %s -check-prefix=CALLERS-F
//
// RUN: infile="%s"; %capi -i 'on_call_path_from(by_name("a", %%%%))' -o %s_callees_a.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_callees_a.filt | c++filt | sort | %filecheck %s -check-prefix=CALLEES-A
//
// RUN: infile="%s"; %capi -i 'on_call_path_from(by_name("f", %%%%))' -o %s_callees_f.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_callees_f.filt | c++filt | sort | %filecheck %s -check-prefix=CALLEES-F
//
// clang-format on

extern "C" {

void a();
void b();
void c();
void d();
void e();
void f();

void a() { b(); d(); f(); }
void b() { c(); f();}
void c() { b(); }
void d() { e(); }
void e() { a(); }
void f() {}
}

// LOG: Running graph analysis
// LOG: Number of SCCs: 3
// LOG: Largest SCC: 3
// LOG: Number of SCCs containing more than 1 node: 2
// LOG: Number of SCCs containing more than 2 nodes: 1

// CALLERS-A: a
// CALLERS-A-NOT: b
// CALLERS-A-NOT: c
// CALLERS-A: d
// CALLERS-A: e
// CALLERS-A-NOT: f

// CALLERS-F: a
// CALLERS-F: b
// CALLERS-F: c
// CALLERS-F: d
// CALLERS-F: e
// CALLERS-F: f

// CALLEES-A: a
// CALLEES-A: b
// CALLEES-A: c
// CALLEES-A: d
// CALLEES-A: e
// CALLEES-A: f

// CALLEES-F-NOT: a
// CALLEES-F-NOT: b
// CALLEES-F-NOT: c
// CALLEES-F-NOT: d
// CALLEES-F-NOT: e
// CALLEES-F: f

