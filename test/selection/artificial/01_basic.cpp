// clang-format off
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; %capi -i 'byName("(main)|(_Z1ai)", %%%%)' -o %s_name.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_name.filt | c++filt | sort | %filecheck %s -check-prefix=NAME
//
// RUN: infile="%s"; %capi -i 'byName("^(?!_Z1).*", %%%%)' -o %s_name2.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_name2.filt | c++filt | sort | %filecheck %s -check-prefix=NAME2
//
// RUN: infile="%s"; %capi -i 'loopDepth(">=", 1, %%%%)' -o %s_loop.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_loop.filt | c++filt | sort | %filecheck %s -check-prefix=LOOP
//
// RUN: infile="%s"; %capi -i 'loopDepth("==", 2, %%%%)' -o %s_loop2.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_loop2.filt | c++filt | sort | %filecheck %s -check-prefix=LOOP2
//
// TODO: Global loop depth selection not implemented
// infile="%s"; %capi -i 'globalLoopDepth(">", 2, %%%%)' -o %s_gloop.filt --output-format simple ${infile%%.*}.ipcg
// cat %s_gloop.filt | c++filt | sort | %filecheck %s -check-prefix=GLOOP
//
// RUN: infile="%s"; %capi -i 'flops(">=", 1, %%%%)' -o %s_flops.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_flops.filt | c++filt | sort | %filecheck %s -check-prefix=FLOPS
//
// RUN: infile="%s"; %capi -i 'onCallPathTo(loopDepth("==", 2, %%%%))' -o %s_callers_b.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_callers_b.filt | c++filt | sort | %filecheck %s -check-prefix=CALLERS-B
//
// RUN: infile="%s"; %capi -i 'onCallPathTo(loopDepth("==", 2, %%%%))' -o %s_callers_b.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_callers_b.filt | c++filt | sort | %filecheck %s -check-prefix=CALLERS-B
//
// RUN: infile="%s"; %capi -i 'onCallPathFrom(loopDepth("==", 2, %%%%))' -o %s_callees_b.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_callees_b.filt | c++filt | sort | %filecheck %s -check-prefix=CALLEES-B
//
// RUN: infile="%s"; %capi -i 'minCallDepth(%%%%, ">=", 2)' -o %s_call_depth.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_call_depth.filt | c++filt | sort | %filecheck %s -check-prefix=CALLDEPTH
//
// RUN: infile="%s"; %capi -i 'minCallDepth(%%%%, "<=", 1)' -o %s_call_depth2.filt --output-format simple ${infile%%.*}.ipcg
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

// NAME: a
// NAME-NOT: b
// NAME-NOT: c
// NAME: main

// NAME2-NOT: a
// NAME2-NOT: b
// NAME2-NOT: c
// NAME2: main

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



