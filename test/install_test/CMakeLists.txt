include(CTest)

set(INSTALL_TEST_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE STRING "Install directory of CaPI to be used for the install integration test")

add_test(
        NAME capi_test_install_configure
        COMMAND ${CMAKE_COMMAND}
        -S ${CMAKE_SOURCE_DIR}/test/install_test/test_app
        -B ${CMAKE_BINARY_DIR}/install_test_build
        -DCMAKE_PREFIX_PATH=${INSTALL_TEST_PREFIX}
)
add_test(
        NAME capi_test_install_build
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/install_test_build
)
add_test(
        NAME capi_test_install_run
        COMMAND ${CMAKE_BINARY_DIR}/install_test_build/install_test_app
)