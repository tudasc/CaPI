#include <stdio.h>

void foo(int n) {
  __cyg_profile_func_enter((void*)foo, 0);
  if (n >= 0) {
    printf("%d\n", n);
    foo(n-1);
  }
  __cyg_profile_func_exit((void*)foo, 0);
}

int main() {
  __cyg_profile_func_enter((void*)main, 0);
  foo(5);
  __cyg_profile_func_exit((void*)main, 0);
  return 0;
}
