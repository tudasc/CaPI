#include "main.h"

extern "C" const char* greeting(A* a) {
  return a->greeting();
}

