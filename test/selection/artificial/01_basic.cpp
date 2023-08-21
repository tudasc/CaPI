// clang-format off
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; %capi -i 'by_name(".*", %%%%)' -o /dev/null --print-scc-stats ${infile%%.*}.ipcg | %filecheck %s -check-prefix=LOG
//
// RUN: infile="%s"; %capi -i 'by_name("(main)|(a)", %%%%)' -o %s_name.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_name.filt | c++filt | sort | %filecheck %s -check-prefix=NAME
//
// RUN: infile="%s"; %capi -i 'by_name("@(main)|(_Z1ai)", %%%%)' -o %s_name.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_name.filt | c++filt | sort | %filecheck %s -check-prefix=NAME-MANGLED
//
// RUN: infile="%s"; %capi -i 'by_name("(main)|(a)", %%%%)' -o %s_name.json --output-format json ${infile%%.*}.ipcg
// RUN: cat %s_name.json | c++filt | %filecheck %s -check-prefix=NAME-JSON
//
// RUN: infile="%s"; %capi -i 'by_name("@(main)|(_Z1ai)", %%%%)' -o %s_name.json --output-format json ${infile%%.*}.ipcg
// RUN: cat %s_name.json | c++filt | %filecheck %s -check-prefix=NAME-JSON-MANGLED
//
// RUN: infile="%s"; %capi -i 'by_name("@^(?!_Z1).*", %%%%)' -o %s_name2.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_name2.filt | c++filt | sort | %filecheck %s -check-prefix=NAME2-MANGLED
//
// RUN: infile="%s"; %capi -i 'loop_depth(">=", 1, %%%%)' -o %s_loop.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_loop.filt | c++filt | sort | %filecheck %s -check-prefix=LOOP
//
// RUN: infile="%s"; %capi -i 'loop_depth("==", 2, %%%%)' -o %s_loop2.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_loop2.filt | c++filt | sort | %filecheck %s -check-prefix=LOOP2
//
// TODO: Global loop depth selection not implemented
// infile="%s"; %capi -i 'global_loop_depth(">", 2, %%%%)' -o %s_gloop.filt --output-format simple ${infile%%.*}.ipcg
// cat %s_gloop.filt | c++filt | sort | %filecheck %s -check-prefix=GLOOP
//
// RUN: infile="%s"; %capi -i 'flops(">=", 1, %%%%)' -o %s_flops.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_flops.filt | c++filt | sort | %filecheck %s -check-prefix=FLOPS
//
// RUN: infile="%s"; %capi -i 'on_call_path_to(loop_depth("==", 2, %%%%))' -o %s_callers_b.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_callers_b.filt | c++filt | sort | %filecheck %s -check-prefix=CALLERS-B
//
// RUN: infile="%s"; %capi -i 'on_call_path_to(loop_depth("==", 2, %%%%))' -o %s_callers_b.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_callers_b.filt | c++filt | sort | %filecheck %s -check-prefix=CALLERS-B
//
// RUN: infile="%s"; %capi -i 'on_call_path_from(loop_depth("==", 2, %%%%))' -o %s_callees_b.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_callees_b.filt | c++filt | sort | %filecheck %s -check-prefix=CALLEES-B
//
// RUN: infile="%s"; %capi -i 'min_call_depth(%%%%, ">=", 2)' -o %s_call_depth.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_call_depth.filt | c++filt | sort | %filecheck %s -check-prefix=CALLDEPTH
//
// RUN: infile="%s"; %capi -i 'min_call_depth(%%%%, "<=", 1)' -o %s_call_depth2.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_call_depth2.filt | c++filt | sort | %filecheck %s -check-prefix=CALLDEPTH2
//
// clang-format on

float c(int i, int j) {
  float t = (float) i * (float )j + 0.5;
  return t;
}

void b(int n) {
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      c(i, j);
    }
  }
}

void a(int n) {
  for (int i = 0; i < n; i++) {
    b(i);
  }
}

int main(int argc, char** argv) {
  a(argc);
  c(0, 1);
  return 0;
}

// LOG: Running graph analysis
// LOG: Number of SCCs: 4
// LOG: Largest SCC: 1
// LOG: Number of SCCs containing more than 1 node: 0
// LOG: Number of SCCs containing more than 2 nodes: 0

// NAME: a
// NAME-NOT: b
// NAME-NOT: c
// NAME: main

// NAME-MANGLED: a
// NAME-MANGLED-NOT: b
// NAME-MANGLED-NOT: c
// NAME-MANGLED: main

// NAME-JSON: a
// NAME-JSON-NOT: b
// NAME-JSON-NOT: c
// NAME-JSON: main

// NAME-JSON-MANGLED: a
// NAME-JSON-MANGLED-NOT: b
// NAME-JSON-MANGLED-NOT: c
// NAME-JSON-MANGLED: main

// NAME2-MANGLED-NOT: a
// NAME2-MANGLED-NOT: b
// NAME2-MANGLED-NOT: c
// NAME2-MANGLED: main

// LOOP: a
// LOOP: b
// LOOP-NOT: c
// LOOP-NOT: main

// LOOP2-NOT: a
// LOOP2: b
// LOOP2-NOT: c
// LOOP2-NOT: main

// GLOOP-NOT: a
// GLOOP-NOT: b
// GLOOP: c
// GLOOP-NOT: main

// FLOPS-NOT: a
// FLOPS-NOT: b
// FLOPS: c
// FLOPS-NOT: main

// CALLERS-B: a
// CALLERS-B: b
// CALLERS-B-NOT: c
// CALLERS-B: main

// CALLEES-B-NOT: a
// CALLEES-B: b
// CALLEES-B: c
// CALLEES-B-NOT: main

// CALLDEPTH-NOT: a
// CALLDEPTH: b
// CALLDEPTH-NOT: c
// CALLDEPTH-NOT: main

// CALLDEPTH2: a
// CALLDEPTH2-NOT: b
// CALLDEPTH2: c
// CALLDEPTH2: main



