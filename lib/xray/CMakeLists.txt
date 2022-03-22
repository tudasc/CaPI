
set(DYNCAPI_SOURCES
        XRayRuntime.cpp
        SymbolRetriever.cpp
        ../selection/FunctionFilter.cpp)

# TODO: Is there a better way to determine this include path?
set(XRAY_INCLUDE_DIR ${LLVM_LIBRARY_DIR}/clang/${LLVM_VERSION}/include)
#message("XRay include dir:  ${XRAY_INCLUDE_DIR}")



#if (SCOREP_SUPPORT)
#    list(APPEND DYNCAPI_SOURCES ScorePInit.cpp)
#endif()

add_library(dyncapi STATIC ${DYNCAPI_SOURCES})
target_include_directories(dyncapi PRIVATE ${XRAY_INCLUDE_DIR})

if (SCOREP_SUPPORT)
    set(DYNCAPI_SCOREP_SOURCES ${DYNCAPI_SOURCES} ScorePInit.cpp)
    add_library(dyncapi_scorep STATIC ${DYNCAPI_SCOREP_SOURCES})
    target_include_directories(dyncapi_scorep PRIVATE ${XRAY_INCLUDE_DIR})
    target_compile_definitions(dyncapi_scorep PRIVATE "-DCAPI_SCOREP_SUPPORT")
    target_link_libraries(dyncapi_scorep PRIVATE ${scorep_mgmt} ${scorep_measurement})
endif()


