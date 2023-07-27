// clang-format off
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
//
// RUN: infile="%s"; %capi -i 'onCallPathFrom(byName("foo", %%%%))' -o %s_down.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_down.filt | c++filt | sort | %filecheck %s -check-prefix=DOWN
//
// RUN: infile="%s"; %capi -i 'onCallPathFrom(byName("foo", %%%%))' -o %s_down_v.filt --output-format simple --traverse-virtual-dtors ${infile%%.*}.ipcg
// RUN: cat %s_down_v.filt | c++filt | sort | %filecheck %s -check-prefix=DOWN-VIRTUAL
//
// RUN: infile="%s"; %capi -i 'onCallPathTo(byName("@_ZN1CD0Ev", %%%%))' -o %s_up.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s_up.filt | c++filt | sort | %filecheck %s -check-prefix=UP
//
// RUN: infile="%s"; %capi -i 'onCallPathTo(byName("@_ZN1CD0Ev", %%%%))' -o %s_up_v.filt --output-format simple --traverse-virtual-dtors ${infile%%.*}.ipcg
// RUN: cat %s_up_v.filt | c++filt | sort | %filecheck %s -check-prefix=UP-VIRTUAL
//
// clang-format on

struct A {
  virtual ~A() {}
};

struct B: public A {
  virtual ~B() {}
};

struct C: public B {
  virtual ~C() {}
};

void foo(A* a) {
  delete a;
}

// DOWN: A::~A
// DOWN-NOT: B::~B
// DOWN-NOT: C::~C
// DOWN: foo

// DOWN-VIRTUAL: A::~A
// DOWN-VIRTUAL: B::~B
// DOWN-VIRTUAL: C::~C
// DOWN-VIRTUAL: foo

// UP-NOT: A::~A
// UP-NOT: B::~B
// UP: C::~C
// UP-VIRTUAL-NOT: foo

// UP-VIRTUAL-NOT: A::~A
// UP-VIRTUAL-NOT: B::~B
// UP-VIRTUAL: C::~C
// UP-VIRTUAL: foo




