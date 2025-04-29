find_package(metacg 0.2 REQUIRED)
message(STATUS "Found MetaCG: ${metacg_DIR}")

get_target_property(METACG_INCLUDE_DIR metacg::metacg INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Include directories for MetaCG: ${METACG_INCLUDE_DIR}")


find_program(CGC_EXE
        NAMES cgcollector
        HINTS "${metacg_DIR}/../../../bin"
        REQUIRED
        )

message(STATUS "CGCollector: ${CGC_EXE}")