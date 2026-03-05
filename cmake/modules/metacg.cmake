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

get_target_property(METACG_LIBRARY metacg::metacg LOCATION)
get_target_property(METACG_INCLUDE_DIR metacg::metacg INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Include directories for MetaCG: ${METACG_INCLUDE_DIR}")

find_library(CAGE_LIB_PATH
        NAMES libcage.a
        PATHS ${metacg_DIR}/../..
        NO_DEFAULT_PATH
)

find_library(CAGE_MD_LIB_PATH
        NAMES libcage_metadata.so
        PATHS ${metacg_DIR}/../..
        NO_DEFAULT_PATH
)

if(CAGE_LIB_PATH)
    message(STATUS "Found CaGe: ${CAGE_LIB_PATH}")
    add_library(cage STATIC IMPORTED)
    set_target_properties(cage PROPERTIES
            IMPORTED_LOCATION ${CAGE_LIB_PATH}
    )
else()
    message(FATAL_ERROR "CaGe not found")
endif()

if(CAGE_LIB_PATH)
    message(STATUS "Found CaGe metadata: ${CAGE_MD_LIB_PATH}")
    add_library(cage_metadata STATIC IMPORTED)
    set_target_properties(cage_metadata PROPERTIES
            IMPORTED_LOCATION ${CAGE_MD_LIB_PATH}
    )
else()
    message(FATAL_ERROR "CaGe metadata not found")
endif()

function(add_metacg target)
    target_link_libraries(${target} PRIVATE metacg::metacg)
endfunction()

function(add_cage target)
    target_link_libraries(${target} PRIVATE "-Wl,--whole-archive" cage "-Wl,--no-whole-archive")
endfunction()
