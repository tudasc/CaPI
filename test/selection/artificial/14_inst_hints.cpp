// clang-format off
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; %capi -i 'sel=by_name("(main)|(a)", %%%%) !instrument(%%sel)' -o %s_inst.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_inst.filt | c++filt | sort | %filecheck %s -check-prefix=INST
//
// RUN: infile="%s"; %capi -i 'start=by_name(".*c.*", %%%%) sel=by_name("(main)|(a)", %%%%) !instrument(%%sel) !begin_after(%%start)' -o %s_inst2.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_inst2.filt | c++filt | sort | %filecheck %s -check-prefix=INST

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

// INST: a
// INST-NOT: b
// INST-NOT: c
// INST: main