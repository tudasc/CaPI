// clang-format off
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; %capi -i 'onCallPathFrom(byName("main", %%%%))' -o %s.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s.filt | c++filt | sort | %filecheck %s
//
// clang-format on

struct A {
  virtual void foo() {};
};

struct B: public A {
  void foo() override {}
};

struct C: public B {
  void foo() final {};
};

int main(int argc, char** argv) {
  // This is of course nonsense, but prevents any kind of inference of the subclass.
  A* a = (A*) (argv[0]);
  a->foo();
  return 0;
}

// CHECK: A::foo
// CHECK: B::foo
// CHECK: C::foo
// CHECK: main