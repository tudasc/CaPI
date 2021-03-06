find_package(LLVM 10 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include(FetchContent)
include(json)
include(AddLLVM)
include(llvm-lit)
include(clang-tidy)
include(clang-format)
include(llvm-util)
include(talp)


if (NOT CMAKE_BUILD_TYPE)
  # set default build type
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
  message(STATUS "Building as debug (default)")
endif ()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  # set default install path
  set(CMAKE_INSTALL_PREFIX "${instro-llvm_SOURCE_DIR}/install/instro-llvm" CACHE PATH "Default install path" FORCE)
  message(STATUS "Installing to (default): ${CMAKE_INSTALL_PREFIX}")
endif ()

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
