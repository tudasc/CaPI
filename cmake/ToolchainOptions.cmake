find_package(LLVM 13 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

#find_package(Clang REQUIRED CONFIG)
#message(STATUS "Found Clang ${CLANG_PACKAGE_VERSION}")

find_package(MPI REQUIRED)
message(STATUS "Found MPI ${MPI_C_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include(FetchContent)
include(json)
include(AddLLVM)
include(clang-tidy)
include(clang-format)
include(llvm-util)

include(talp)

option(ENABLE_TESTING "Enable testing (requires lit)" ON)

if (ENABLE_TESTING)
  include(metacg)

  find_llvm_progs(FILECHECK_EXE "FileCheck-${LLVM_VERSION_MAJOR};FileCheck" ABORT_IF_MISSING)

  if(LLVM_EXTERNAL_LIT)
    cmake_path(GET LLVM_EXTERNAL_LIT PARENT_PATH LLVM_EXTERNAL_LIT_DIR)
  endif()

  find_llvm_progs(LIT_EXE
          "llvm-lit;lit;lit.py"
          HINTS ${LLVM_EXTERNAL_LIT_DIR} /usr/lib/llvm-${LLVM_VERSION_MAJOR} /usr/lib/llvm /usr/bin /usr/local/bin /opt/local/bin
          ABORT_IF_MISSING
          )

  message(STATUS "Found llvm-lit: ${LIT_EXE}")
endif()

option(ENABLE_XRAY "Enable XRay dynamic instrumentation interface" ON)
option(SCOREP_SUPPORT "Enable Score-P support" ON)

if (SCOREP_SUPPORT)
  set(SCOREP_PATH "" CACHE PATH "Path to ScoreP installation")

  find_library(scorep_mgmt  NAMES scorep_adapter_compiler_mgmt HINTS "${SCOREP_PATH}/lib"  REQUIRED)
  find_library(scorep_measurement  NAMES scorep_measurement HINTS "${SCOREP_PATH}/lib" REQUIRED)

  message(STATUS "Score-P libs: ${scorep_mgmt} ${scorep_measurement}")
endif()

option(ENABLE_EXTRAE "Enable Extrae tracing interface" ON)

if (NOT CMAKE_BUILD_TYPE)
  # set default build type
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
  message(STATUS "Building as debug (default)")
endif ()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  # set default install path
  set(CMAKE_INSTALL_PREFIX "${capi_SOURCE_DIR}/install/capi" CACHE PATH "Default install path" FORCE)
  message(STATUS "Installing to (default): ${CMAKE_INSTALL_PREFIX}")
endif ()

function(add_version_include target)
  target_include_directories(${target} PUBLIC "${PROJECT_BINARY_DIR}")
endfunction()

function(target_project_compile_options target)
  cmake_parse_arguments(ARG "" "" "PRIVATE_FLAGS;PUBLIC_FLAGS" ${ARGN})

  target_compile_options(${target} PRIVATE
    -Wall -Wextra -pedantic
    -Wunreachable-code -Wwrite-strings
    -Wpointer-arith -Wcast-align
    -Wcast-qual -Wno-unused-parameter
    )

  if (ARG_PRIVATE_FLAGS)
    target_compile_options(${target} PRIVATE
      "${ARG_PRIVATE_FLAGS}"
      )
  endif ()

  if (ARG_PUBLIC_FLAGS)
    target_compile_options(${target} PUBLIC
      "${ARG_PUBLIC_FLAGS}"
      )
  endif ()
endfunction()

function(target_project_compile_definitions target)
  cmake_parse_arguments(ARG "" "" "PRIVATE_DEFS;PUBLIC_DEFS" ${ARGN})

  if (ARG_PRIVATE_DEFS)
    target_compile_definitions(${target} PRIVATE
      "${ARG_PRIVATE_DEFS}"
      )
  endif ()

  if (ARG_PUBLIC_DEFS)
    target_compile_definitions(${target} PUBLIC
      "${ARG_PUBLIC_DEFS}"
      )
  endif ()
endfunction()
