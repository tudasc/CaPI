
# Note: This file is currently not in use, until we find a way to have CMake not overwrite our custom generated Makefile

file(GLOB LULESH_SOURCES *.cc *.h)
file(GLOB CAPI_FILES *.capi)

file(COPY ${LULESH_SOURCES} ${CAPI_FILES} lulesh_full.mcg CAPI_README DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


set(CAPI_LIB_DIR ${CMAKE_BINARY_DIR}/lib)
set(CC_PATH ${LLVM_BINARY_DIR}/bin/clang)
set(CXX_PATH ${LLVM_BINARY_DIR}/bin/clang++)
set(LLVM_CONFIG ${LLVM_BINARY_DIR}/bin/llvm-config)
set(SCOREP_CONFIG ${SCOREP_DIR}/bin/scorep-config)

configure_file(Makefile.in Makefile @ONLY)