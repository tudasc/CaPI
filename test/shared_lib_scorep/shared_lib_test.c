#include <stdio.h>
#include <stdlib.h>

#include "testlib.h"

void foo(int n) {
    if (n >= 0) {
        printf("%d\n", n);
        foo(n-1);
    }
}

int main(int argc, char** argv) {
    printf("addr of lib_fn is %p\n", lib_fn);
    int x = lib_fn();
    foo(x);
    return 0;
}
