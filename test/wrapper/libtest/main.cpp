//
// Created by sebastian on 28.01.26.
//


#include <iostream>

extern "C" const char* greeting();

int main(int argc, char** argv) {
  std::cout << "This is a test application!\n";
  std::cout << "Shared library says: " << greeting() << "\n";
  return 0;
}
