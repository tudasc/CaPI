add_library(test-rt STATIC test-rt.cpp)

add_subdirectory(passes)
add_subdirectory(selection)
add_subdirectory(integration)

file(GLOB INTEGRATION_EXECUTABLES integration/simpleTester.cpp integration/runTestCases integration/simpleTester integration/testfunctions)
file(GLOB INTEGRATION_INPUT integration/input)
file(GLOB TEMP_JSON json.hpp)

file(COPY ${INTEGRATION_EXECUTABLES} ${INTEGRATION_INPUT} ${TEMP_JSON} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/integration)

