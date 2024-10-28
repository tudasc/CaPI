//
// Created by sebastian on 28.10.24.
//

// RUN: %clang_cxx -fPIC %test_flags %s -o %s.o
// RUN: %s.o 2>&1 | FileCheck %s

#include "SymbolRetrieverTestRT.h"
#include <stdlib.h>

int main(int argc, char** argv) {
  load_symbols();
  // CHECK: malloc
  // CHECK-NOT: not found
  check_symbol((void*)malloc);
  // CHECK: main
  // CHECK-NOT: not found
  check_symbol((void*)main);
  return 0;
}