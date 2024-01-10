// clang-format off
// Note: This code doesn't do anything useful and is not meant to be run.
//
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; %capi -i 'common_caller(by_name("a", %%%%), by_name("b", %%%%))' -o %s.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s.filt | c++filt | sort | %filecheck %s
//
// clang-format on



void a() {}
void b() {}

void e();

struct A {
  virtual void fuckery() {
    a();
    b();
    e();
  }
};

struct B: A {
  void fuckery() override {
    A::fuckery();
  }
};

void d() {
  b();
}

void c() {
  a();
}


void e() {
  c();
  d();
}

// CHECK: A::fuckery
// CHECK-NOT: B::fuckery
// CHECK: a
// CHECK: b
// CHECK: c
// CHECK: d
// CHECK: e


