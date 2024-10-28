//
// Created by sebastian on 28.10.24.
//

// RUN: %clang_cxx -fPIC %test_flags %s -o %s.o
// RUN: %s.o | FileCheck %s

#include "SymbolRetrieverTestRT.h"

int main(int argc, char** argv) {
  // CHECK: main
  // CHECK-NOT: not found
  check_symbol((void*)main);
  return 0;
}