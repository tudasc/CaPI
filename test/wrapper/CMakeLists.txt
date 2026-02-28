set(CC ${LLVM_BINARY_DIR}/bin/clang)
set(CXX ${LLVM_BINARY_DIR}/bin/clang++)
set(CAPICC ${CMAKE_BINARY_DIR}/scripts/capicc)

file(GLOB TEST_VARIANTS "libtest/Makefile.*.include")
file(COPY libtest/shared.cpp libtest/shared2.cpp libtest/main.cpp ${TEST_VARIANTS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
configure_file(libtest/Makefile.in Makefile @ONLY)