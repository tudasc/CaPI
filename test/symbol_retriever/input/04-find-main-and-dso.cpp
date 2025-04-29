//
// Created by sebastian on 28.10.24.
//

// RUN: split-file %s %t
//
// RUN: %clang_cxx %extra_cxx_flags  -fPIC -fuse-ld=lld -flto=thin -shared %t/test_dso.cpp -o %t/test_dso.so
// RUN: %clang_cxx %extra_cxx_flags  -fPIC %test_flags %t/main.cpp %t/test_dso.so -Wl,-rpath %t  -o %t/main.o
// RUN: %t/main.o | FileCheck %t/main.cpp
//
// RUN: %clang_cxx %extra_cxx_flags  -fPIC -fuse-ld=lld -flto=thin -shared %t/test_dso.cpp -o %t/test_dso.so
// RUN: %clang_cxx %extra_cxx_flags  -fPIC -fuse-ld=lld -flto=thin %test_flags %t/main.cpp %t/test_dso.so -Wl,-rpath %t  -o %t/main.o
// RUN: %t/main.o | FileCheck %t/main.cpp

//--- main.cpp

#include "SymbolRetrieverTestRT.h"

extern "C" void* foo();

int main(int argc, char** argv) {
  load_symbols();
  // CHECK: main
  // CHECK-NOT: not found
  check_symbol((void*)main);
  // CHECK: foo called
  auto addr = foo();
  // CHECK: foo
  // CHECK-NOT: not found
  check_symbol((void*)foo);
  check_symbol(addr);
  return 0;
}

//--- test_dso.cpp
#include <iostream>

extern "C" void* foo() {
  std::cout << "foo called" << std::endl;
  return (void*) foo;
}
