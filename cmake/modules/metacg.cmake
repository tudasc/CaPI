find_package(metacg 0.2 REQUIRED)
message(STATUS "Found MetaCG: ${metacg_DIR}")


find_program(CGC_EXE
        NAMES cgcollector
        HINTS "${metacg_DIR}/../../../bin"
        REQUIRED
        )

message(STATUS "CGCollector: ${CGC_EXE}")