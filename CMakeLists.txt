cmake_minimum_required(VERSION 3.15)
project(capi VERSION 0.3)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(-fno-rtti)

list(APPEND CMAKE_MODULE_PATH
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

include(CapiToolchainOptions)
#include(CMakePackageConfigHelpers)

add_format_target(format-sources
        "Formats project source files"
        TARGETS
        src/*.cpp
        src/*.h
        )


add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(examples)

if (ENABLE_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()


include(GetGitRevisionDescription)

get_git_head_revision(GIT_REFSPEC GIT_SHA1)
if(NOT DEFINED GIT_SHA1)
    set(GIT_SHA1 "NO_GIT_ROOT")
endif()
configure_file(capi_version.h.in capi_version.h)


# Install rules

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include)

install(EXPORT capiTargets
        FILE capiTargets.cmake
        NAMESPACE capi::
        DESTINATION lib/cmake/capi
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/capiConfigVersion.cmake"
        VERSION 1.0.0 # TODO: Adjust
        COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/capiConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/capiConfig.cmake"
        INSTALL_DESTINATION lib/cmake/capi
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/capiConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/capiConfigVersion.cmake"
        DESTINATION lib/cmake/capi
)