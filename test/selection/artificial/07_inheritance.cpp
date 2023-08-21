// clang-format off
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; %capi -i 'on_call_path_from(by_name("main", %%%%))' -o %s_down.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_down.filt | c++filt | sort | %filecheck %s -check-prefix=DOWN
//
// RUN: infile="%s"; %capi -i 'on_call_path_to(by_name("C::foo", %%%%))' -o %s_up.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_up.filt | c++filt | sort | %filecheck %s -check-prefix=UP
//
// RUN: infile="%s"; %capi -i 'on_call_path_to(by_name("@_ZN1C3fooEv", %%%%))' -o %s_up.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_up.filt | c++filt | sort | %filecheck %s -check-prefix=UP-MANGLED
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
  // This is of course nonsense, but prevents any kind of inference of the direct call target.
  A* a = (A*) (argv[0]);
  a->foo();
  return 0;
}

// DOWN: A::foo
// DOWN: B::foo
// DOWN: C::foo
// DOWN: main

// UP-NOT: A::foo
// UP-NOT: B::foo
// UP: C::foo
// UP: main

// UP-MANGLED-NOT: A::foo
// UP-MANGLED-NOT: B::foo
// UP-MANGLED: C::foo
// UP-MANGLED: main
