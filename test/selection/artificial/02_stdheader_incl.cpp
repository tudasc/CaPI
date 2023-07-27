// clang-format off
// RUN: LD_LIBRARY_PATH="$(dirname %cgc)/../lib:$LD_LIBRARY_PATH" %cgc --capture-ctors-dtors --extra-arg=-I%clang_include_dir --metacg-format-version=2 %s
// RUN: infile="%s"; %capi -i 'subtract(%%%%, join(inSystemHeader(%%%%), byName("@_ZNKSt4hash.*", %%%%)))' -o %s.filt --output-format simple ${infile%%.*}.ipcg
// RUN: cat %s.filt | c++filt | sort | %filecheck %s
// clang-format on

#include <vector>
#include <iostream>

int main(int argc, char** argv) {
  std::vector<int> v;
  v.push_back(argc);

  std::cout << "Hello world!" << std::endl;
  return 0;
}


// CHECK: main
// CHECK-NOT: std::
