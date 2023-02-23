
set(CAPI_XRAY_SOURCES
        XRayRuntime.cpp
        ../symbol_retriever/SymbolRetriever.cpp
        ../selection/FunctionFilter.cpp)

# TODO: Is there a better way to determine this include path?
set(XRAY_INCLUDE_DIR ${LLVM_LIBRARY_DIR}/clang/${LLVM_VERSION}/include)
#message("XRay include dir:  ${XRAY_INCLUDE_DIR}")


add_library(capixray_gnu STATIC ${CAPI_XRAY_SOURCES} GNUInterface.cpp)
target_include_directories(capixray_gnu PRIVATE ${XRAY_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../symbol_retriever)
install(TARGETS capixray_gnu ARCHIVE DESTINATION lib)

if (ENABLE_TALP)

    set(CAPI_XRAY_TALP_SOURCES ${CAPI_XRAY_SOURCES} TALPInterface.cpp)
    add_library(capixray_talp STATIC ${CAPI_XRAY_TALP_SOURCES})
    target_include_directories(capixray_talp PRIVATE ${XRAY_INCLUDE_DIR} ${DLB_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../symbol_retriever)
    target_compile_definitions(capixray_talp PRIVATE "-DCAPI_TALP_INTERFACE")
    target_link_libraries(capixray_talp PUBLIC ${dlb_mpi_lib})
    install(TARGETS capixray_talp ARCHIVE DESTINATION lib)
endif()


if (SCOREP_SUPPORT)
    set(CAPI_XRAY_SCOREP_SOURCES ${CAPI_XRAY_SOURCES} GNUInterface.cpp ScorePInit.cpp)
    add_library(capixray_scorep STATIC ${CAPI_XRAY_SCOREP_SOURCES})
    target_include_directories(capixray_scorep PRIVATE ${XRAY_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../symbol_retriever)
    target_compile_definitions(capixray_scorep PRIVATE "-DCAPI_SCOREP_INTERFACE")
    target_link_libraries(capixray_scorep PRIVATE ${scorep_mgmt} ${scorep_measurement})
    install(TARGETS capixray_scorep ARCHIVE DESTINATION lib)
endif()


