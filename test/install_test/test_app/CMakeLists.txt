cmake_minimum_required(VERSION 3.16)
project(capi_integration_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(metacg REQUIRED)
find_package(capi REQUIRED)

add_executable(install_test_app InstallTestApp.cpp)

message(STATUS "Building integration test with: ${CMAKE_CXX_COMPILER}")

target_link_libraries(install_test_app PRIVATE capi::capi_select)