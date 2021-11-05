//
// Created by sebastian on 15.10.21.
//


// TODO: Testing setup
// RUN: clang-inst++ -mllvm -metacg-file="basic.ipcg" -O2 basic.cpp -ltest-rt -o basic | ./basic

#include "test-rt.h"

#include <stdio.h>

int foo(int x) {
    if (x < 1000) {
        return foo(x * 2 + 1);
    }
    return x;
}

int main(int argc, char** argv) {
   init_test_rt(argc, argv);
   int result = foo(argc);
   return result;
}





