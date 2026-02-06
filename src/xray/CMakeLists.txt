
set(CAPI_XRAY_SOURCES
        XRayRuntime.cpp
        CallLogger.cpp
        ../symbol_retriever/SymbolRetriever.cpp
        ../selection/FunctionFilter.cpp
        ../selection/MeasurementConfig.cpp
        ../selection/MeasurementConfigIO.cpp)

add_library(capi_dso_init STATIC DSOInit.cpp)
install(TARGETS capi_dso_init ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_library(capixray_gnu STATIC ${CAPI_XRAY_SOURCES} GNUInterface.cpp)
add_json(capixray_gnu)
add_cxxopts(capixray_gnu)
target_include_directories(capixray_gnu PRIVATE ${CLANG_INCLUDE_DIR} ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/../symbol_retriever ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src)
add_version_include(capixray_gnu)
target_link_libraries(capixray_gnu PUBLIC ${LLVM_LIBRARY_DIR}/lib/libLLVMXRay.a)
install(TARGETS capixray_gnu ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if (ENABLE_TALP)

    set(CAPI_XRAY_TALP_SOURCES ${CAPI_XRAY_SOURCES} TALPInterface.cpp)
    add_library(capixray_talp STATIC ${CAPI_XRAY_TALP_SOURCES})
    add_json(capixray_talp)
    add_cxxopts(capixray_talp)
    target_include_directories(capixray_talp PRIVATE ${CLANG_INCLUDE_DIR} ${LLVM_INCLUDE_DIRS} ${DLB_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../symbol_retriever ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src)
    add_version_include(capixray_talp)
    target_compile_definitions(capixray_talp PRIVATE "-DCAPI_TALP_INTERFACE")
    #target_link_libraries(capixray_talp PUBLIC ${dlb_mpi_lib})
    #target_link_libraries(capixray_talp PUBLIC ${LLVM_LIBRARY_DIR}/lib/libLLVMXRay.a)
    install(TARGETS capixray_talp ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if (ENABLE_SCOREP)
    set(CAPI_XRAY_SCOREP_SOURCES ${CAPI_XRAY_SOURCES} GNUInterface.cpp ScorePInit.cpp)
    add_library(capixray_scorep STATIC ${CAPI_XRAY_SCOREP_SOURCES})
    add_json(capixray_scorep)
    add_cxxopts(capixray_scorep)
    target_include_directories(capixray_scorep PRIVATE ${CLANG_INCLUDE_DIR} ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/../symbol_retriever ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src)
    add_version_include(capixray_scorep)
    target_compile_definitions(capixray_scorep PRIVATE "-DCAPI_SCOREP_INTERFACE")
    #target_link_libraries(capixray_scorep PRIVATE ${scorep_mgmt} ${scorep_measurement})
    install(TARGETS capixray_scorep ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if (ENABLE_EXTRAE)
    set(CAPI_XRAY_EXTRAE_SOURCES ${CAPI_XRAY_SOURCES} ExtraeInterface.cpp)
    add_library(capixray_extrae STATIC ${CAPI_XRAY_EXTRAE_SOURCES})
    add_json(capixray_extrae)
    add_cxxopts(capixray_extrae)
    target_include_directories(capixray_extrae PRIVATE ${MPI_C_INCLUDE_DIRS} ${CLANG_INCLUDE_DIR} ${LLVM_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/../symbol_retriever ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src)
    add_version_include(capixray_extrae)
    target_compile_definitions(capixray_extrae PRIVATE "-DCAPI_EXTRAE_INTERFACE -DWITH_MPI")
    install(TARGETS capixray_extrae ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if (ENABLE_NESMIK)
    set(CAPI_XRAY_NESMIK_SOURCES ${CAPI_XRAY_SOURCES} NeSmiKInterface.cpp)
    add_library(capixray_nesmik STATIC ${CAPI_XRAY_NESMIK_SOURCES})
    add_json(capixray_nesmik)
    add_cxxopts(capixray_nesmik)
    get_target_property(NESMIK_INCLUDE_DIR nesmik::nesmik INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(capixray_nesmik PRIVATE ${MPI_C_INCLUDE_DIRS} ${CLANG_INCLUDE_DIR} ${LLVM_INCLUDE_DIRS} ${NESMIK_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../symbol_retriever ${PROJECT_SOURCE_DIR}/include {PROJECT_SOURCE_DIR}/src)
    add_version_include(capixray_nesmik)
    target_compile_definitions(capixray_nesmik PRIVATE "-DCAPI_NESMIK_INTERFACE -DWITH_MPI")
    install(TARGETS capixray_nesmik ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
