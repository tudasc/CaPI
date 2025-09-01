// clang-format off
//
// Note: This code doesn't do anything useful and is not meant to be run.
//
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; %capi -i 'by_name("testDirectExplicit") |> on_call_path_from' -o %s_direct_down.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_direct_down.filt | c++filt | sort | %filecheck %s -check-prefix=DIRECT-DOWN
//
// RUN: infile="%s"; %capi -i 'by_name("testVirtual") |> on_call_path_from' -o %s_virtual_down.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_virtual_down.filt | c++filt | sort | %filecheck %s -check-prefix=VIRTUAL-DOWN
//
// RUN: infile="%s"; %capi -i 'by_name("bar") |> on_call_path_to' -o %s_up.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_up.filt | c++filt | sort | %filecheck %s -check-prefix=UP
//
// XFAIL: *
//
// clang-format on

void bar() {}

struct A {
  virtual void foo(){};
};

struct B: A {
  void foo() override {}
};

struct C: B {
  void foo() override {}
};

struct D: B {
  void foo() override {bar();}
};

// TODO: Add this as a test case for devirtualization detection
void testDirect() {
  A a;
  a.foo();
}

void testDirectExplicit() {
  A a;
  a.A::foo();
}

void testVirtual(A* a) {
  a->foo();
}

// Note: This is the expected behavior if MetaCG was able to recognize that this call is devirtualized. This is currently not the case.
// DIRECT-DOWN-DEVIRT: A::foo
// DIRECT-DOWN-DEVIRT-NOT: B::foo
// DIRECT-DOWN-DEVIRT-NOT: C::foo
// DIRECT-DOWN-DEVIRT-NOT: D::foo

// DIRECT-DOWN: A::foo
// DIRECT-DOWN-NOT: B::foo
// DIRECT-DOWN-NOT: C::foo
// DIRECT-DOWN-NOT: D::foo
// DIRECT-DOWN-NOT: bar

// VIRTUAL-DOWN: A::foo
// VIRTUAL-DOWN: B::foo
// VIRTUAL-DOWN: C::foo
// VIRTUAL-DOWN: D::foo
// VIRTUAL-DOWN: bar

// UP-NOT: A::foo
// UP-NOT: B::foo
// UP-NOT: C::foo
// UP: D::foo
// UP-NOT: testDirectExplicit
// UP: testVirtual


