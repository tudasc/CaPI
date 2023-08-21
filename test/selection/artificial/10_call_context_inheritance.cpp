// clang-format off
// Note: This code doesn't do anything useful and is not meant to be run.
//
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; %capi -i 'common_caller(by_name("b", %%%%), by_name("c", %%%%))' -o %s.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s.filt | c++filt | sort | %filecheck %s
//
// clang-format on


void c() {}

struct A {
  virtual void fuckery() {c();}
};

struct B: A {
  void fuckery() override {
    A::fuckery();
  }
};

void b() {}

void a() {
  b();
  B* b = new B;
  b->fuckery();
}

// CHECK: B::fuckery
// CHECK: a
// CHECK: b
// CHECK: c


