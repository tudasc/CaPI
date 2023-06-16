
set(CAPI_XRAY_SOURCES
        XRayRuntime.cpp
        ../symbol_retriever/SymbolRetriever.cpp
        ../selection/FunctionFilter.cpp)

# TODO: Is there a better way to determine this include path?
set(XRAY_INCLUDE_DIR ${LLVM_LIBRARY_DIR}/clang/${LLVM_VERSION}/include)
#message("XRay include dir:  ${XRAY_INCLUDE_DIR}")

#message("LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")

#llvm_map_components_to_libnames(llvm_libs_test xray support object)
#message("LLVM Libs for XRay: ${llvm_libs_test}")



add_library(capixray_gnu STATIC ${CAPI_XRAY_SOURCES} GNUInterface.cpp)
target_include_directories(capixray_gnu PRIVATE ${XRAY_INCLUDE_DIR} ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/../symbol_retriever)
target_link_libraries(capixray_gnu PUBLIC ${LLVM_LIBRARY_DIR}/lib/libLLVMXRay.a)
install(TARGETS capixray_gnu ARCHIVE DESTINATION lib)

if (ENABLE_TALP)

    set(CAPI_XRAY_TALP_SOURCES ${CAPI_XRAY_SOURCES} TALPInterface.cpp)
    add_library(capixray_talp STATIC ${CAPI_XRAY_TALP_SOURCES})
    target_include_directories(capixray_talp PRIVATE ${XRAY_INCLUDE_DIR} ${LLVM_INCLUDE_DIRS} ${DLB_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../symbol_retriever)
    target_compile_definitions(capixray_talp PRIVATE "-DCAPI_TALP_INTERFACE")
    #target_link_libraries(capixray_talp PUBLIC ${dlb_mpi_lib})
    #target_link_libraries(capixray_talp PUBLIC ${LLVM_LIBRARY_DIR}/lib/libLLVMXRay.a)
    install(TARGETS capixray_talp ARCHIVE DESTINATION lib)
endif()

if (SCOREP_SUPPORT)
    set(CAPI_XRAY_SCOREP_SOURCES ${CAPI_XRAY_SOURCES} GNUInterface.cpp ScorePInit.cpp)
    add_library(capixray_scorep STATIC ${CAPI_XRAY_SCOREP_SOURCES})
    target_include_directories(capixray_scorep PRIVATE ${XRAY_INCLUDE_DIR} ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/../symbol_retriever)
    target_compile_definitions(capixray_scorep PRIVATE "-DCAPI_SCOREP_INTERFACE")
    #target_link_libraries(capixray_scorep PRIVATE ${scorep_mgmt} ${scorep_measurement})
    install(TARGETS capixray_scorep ARCHIVE DESTINATION lib)
endif()

if (ENABLE_EXTRAE)
    set(CAPI_XRAY_EXTRAE_SOURCES ${CAPI_XRAY_SOURCES} ExtraeInterface.cpp)
    add_library(capixray_extrae STATIC ${CAPI_XRAY_EXTRAE_SOURCES})
    target_include_directories(capixray_extrae PRIVATE ${XRAY_INCLUDE_DIR} ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/../symbol_retriever)
    target_compile_definitions(capixray_extrae PRIVATE "-DCAPI_EXTRAE_INTERFACE")
    install(TARGETS capixray_extrae ARCHIVE DESTINATION lib)
endif()
