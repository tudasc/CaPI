set(EXAMPLES_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB LULESH_SOURCES lulesh/*.cc lulesh/*.h)
file(GLOB CAPI_FILES lulesh/*.capi)

file(COPY ${LULESH_SOURCES} ${CAPI_FILES} lulesh/lulesh_full.mcg lulesh/CAPI_README DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/lulesh)

file(COPY AMG2013 DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(CAPI_LIB_DIR ${CMAKE_BINARY_DIR}/lib)
set(CC_PATH ${LLVM_BINARY_DIR}/bin/clang)
set(CXX_PATH ${LLVM_BINARY_DIR}/bin/clang++)
set(LLVM_CONFIG ${LLVM_BINARY_DIR}/bin/llvm-config)
set(SCOREP_CONFIG ${SCOREP_PATH}/bin/scorep-config)

configure_file(lulesh/Makefile.in lulesh/Makefile @ONLY)

# Note: We can't put this in a separate CMake in the subdirectory, as building with Make will overwrite the generated Makefile.

#add_subdirectory(lulesh)
