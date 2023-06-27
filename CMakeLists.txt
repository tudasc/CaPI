cmake_minimum_required(VERSION 3.15)
project(capi VERSION 0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(-fno-rtti)

list(APPEND CMAKE_MODULE_PATH
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

include(ToolchainOptions)
#include(CMakePackageConfigHelpers)

add_format_target(format-sources
        "Formats project source files"
        TARGETS
        lib/*.cpp
        lib/*.h
        )


add_subdirectory(lib)
add_subdirectory(scripts)
add_subdirectory(test)
add_subdirectory(examples)

if (ENABLE_TESTING)
    enable_testing()
    add_subdirectory(test/selection)
endif()