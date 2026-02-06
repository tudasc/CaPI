set(CC ${LLVM_BINARY_DIR}/bin/clang)
set(CXX ${LLVM_BINARY_DIR}/bin/clang++)
set(CAPICC ${CMAKE_BINARY_DIR}/scripts/capicc)

file(COPY libtest/shared.cpp libtest/shared2.cpp libtest/main.cpp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
configure_file(libtest/Makefile.in Makefile @ONLY)