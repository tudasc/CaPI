# Add option to toggle external MetaCG usage
option(USE_EXTERNAL_METACG "Use system-installed MetaCG instead of fetching it" ON)

if(USE_EXTERNAL_METACG)
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
else()
        message(WARNING "Building MetaCG alongside CaPI is an experimental feature. Please revert to USE_EXTERNAL_METACG if you encounter errors")
        set(METACG_BUILD_CGCOLLECTOR ON CACHE BOOL "" FORCE)
        set(METACG_BUILD_GRAPH_TOOLS ON CACHE BOOL "" FORCE)
        FetchContent_Declare(
                MetaCG
                GIT_REPOSITORY https://github.com/tudasc/MetaCG.git
                GIT_TAG d28254fde4e10e5682c097563d425bff04142510 # Devel
        )
        FetchContent_MakeAvailable(MetaCG)
        FetchContent_GetProperties(MetaCG)
        if(metacg_POPULATED)
            set(CGCOLLECTOR_BINARY_DIR ${MetaCG_BINARY_DIR}/cgcollector/tools)
        else ()
            message(FATAL_ERROR "MetaCG was not populated correctly")
        endif()
        if (ENABLE_TESTING)
            set(CGC_EXE "${CGCOLLECTOR_BINARY_DIR}/cgcollector")
            message(STATUS "CGCollector: ${CGC_EXE}")
        endif ()
endif()

get_target_property(METACG_INCLUDE_DIR metacg::metacg INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "Include directories for MetaCG: ${METACG_INCLUDE_DIR}")
#message(STATUS "Binary directory for CGCollector: ${CGCOLLECTOR_BINARY_DIR}")
