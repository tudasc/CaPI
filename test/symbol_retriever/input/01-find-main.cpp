//
// Created by sebastian on 28.10.24.
//

// RUN: %clang_cxx -fPIC %test_flags %s -o %s.o
// RUN: %s.o | FileCheck %s
//
// RUN: %clang_cxx -fPIC -fuse-ld=lld -flto=thin %test_flags %s -o %s.o
// RUN: %s.o | FileCheck %s

#include "SymbolRetrieverTestRT.h"

int main(int argc, char** argv) {
  load_symbols();
  // CHECK: main
  // CHECK-NOT: not found
  check_symbol((void*)main);
  return 0;
}
