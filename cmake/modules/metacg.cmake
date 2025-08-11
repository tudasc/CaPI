find_package(metacg REQUIRED)

message(STATUS "Found external MetaCG: ${metacg_DIR}")
set(CGCOLLECTOR_BINARY_DIR "${metacg_DIR}/../../../bin")

if (ENABLE_TESTING)
    find_program(CGC_EXE
        NAMES cgcollector
        HINTS "${CGCOLLECTOR_BINARY_DIR}"
        REQUIRED
    )
    message(STATUS "CGCollector: ${CGC_EXE}")
endif ()


get_target_property(METACG_INCLUDE_DIR metacg::metacg INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Include directories for MetaCG: ${METACG_INCLUDE_DIR}")
