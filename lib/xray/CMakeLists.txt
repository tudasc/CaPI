
set(CAPI_XRAY_SOURCES
        XRayRuntime.cpp
        SymbolRetriever.cpp
        ../selection/FunctionFilter.cpp)

# TODO: Is there a better way to determine this include path?
set(XRAY_INCLUDE_DIR ${LLVM_LIBRARY_DIR}/clang/${LLVM_VERSION}/include)
#message("XRay include dir:  ${XRAY_INCLUDE_DIR}")



#if (SCOREP_SUPPORT)
#    list(APPEND DYNCAPI_SOURCES ScorePInit.cpp)
#endif()

add_library(capixray STATIC ${CAPI_XRAY_SOURCES})
target_include_directories(capixray PRIVATE ${XRAY_INCLUDE_DIR})

if (SCOREP_SUPPORT)
    set(CAPI_XRAY_SCOREP_SOURCES ${CAPI_XRAY_SOURCES} ScorePInit.cpp)
    add_library(capixray_scorep STATIC ${CAPI_XRAY_SCOREP_SOURCES})
    target_include_directories(capixray_scorep PRIVATE ${XRAY_INCLUDE_DIR})
    target_compile_definitions(capixray_scorep PRIVATE "-DCAPI_SCOREP_SUPPORT")
    target_link_libraries(capixray_scorep PRIVATE ${scorep_mgmt} ${scorep_measurement})
endif()


