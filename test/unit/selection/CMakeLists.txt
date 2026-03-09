add_executable(selectionTests
        test_analysis.cpp
)

target_link_libraries(selectionTests
        PRIVATE
        capi_select
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(selectionTests)