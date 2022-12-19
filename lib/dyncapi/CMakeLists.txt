
set(DYNCAPI_SOURCES
        ../selection/FunctionFilter.cpp
        ../selection/CallGraph.cpp
        ../selection/MetaCGReader.cpp
        ../selection/DOTWriter.cpp
        ../selection/SpecParser.cpp
        ../selection/SelectionSpecAST.cpp
        ../selection/SelectorBuilder.cpp
        ../selection/SelectorGraph.cpp
        ../selection/selectors/SelectorInstantiation.cpp
        ../selection/selectors/BasicSelectors.cpp
        DynCaPI.cpp
        SymbolRetriever.cpp)

# TODO: Is there a better way to determine this include path?
set(XRAY_INCLUDE_DIR ${LLVM_LIBRARY_DIR}/clang/${LLVM_VERSION}/include)
#message("XRay include dir:  ${XRAY_INCLUDE_DIR}")



#if (SCOREP_SUPPORT)
#    list(APPEND DYNCAPI_SOURCES ScorePInit.cpp)
#endif()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#set(XRAY_PATH "${LLVM_LIBRARY_DIR}/clang/${LLVM_VERSION}/lib/linux/" CACHE PATH "Path to XRay")
#message("XRAY Path: ${XRAY_PATH}")

#find_library(xray  NAMES clang_rt.xray HINTS "${XRAY_PATH}"  REQUIRED)
#find_library(xray-basic  NAMES clang_rt.xray-basic HINTS "${XRAY_PATH}"  REQUIRED)
#find_library(xray-fdr  NAMES clang_rt.xray-fdr HINTS "${XRAY_PATH}"  REQUIRED)
#message("XRay: ${xray}")

add_library(dyncapi STATIC ${DYNCAPI_SOURCES})
message("Src dir: ${CMAKE_CURRENT_SOURCE_DIR}/../selection")
target_include_directories(dyncapi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../selection)
target_include_directories(dyncapi PRIVATE ${XRAY_INCLUDE_DIR})

if (SCOREP_SUPPORT)
    set(DYNCAPI_SCOREP_SOURCES ${DYNCAPI_SOURCES} ScorePInit.cpp)
    add_library(dyncapi_scorep STATIC ${DYNCAPI_SCOREP_SOURCES})
    target_include_directories(dyncapi_scorep PRIVATE ${XRAY_INCLUDE_DIR})
    target_include_directories(dyncapi_scorep PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../selection)
    target_compile_definitions(dyncapi_scorep PRIVATE "-DCAPI_SCOREP_SUPPORT")
    target_link_libraries(dyncapi_scorep PUBLIC ${scorep_mgmt} ${scorep_measurement})
    #target_link_libraries(dyncapi_scorep PUBLIC ${xray} ${xray-basic} ${xray-fdr})
    #target_link_options(dyncapi_scorep PUBLIC -Wl,--whole-archive  ${xray} ${xray-basic} ${xray-fdr} -Wl,--no-whole-archive)

    add_library(dyncapi_scorep_shared SHARED ${DYNCAPI_SCOREP_SOURCES})
    target_include_directories(dyncapi_scorep_shared PRIVATE ${XRAY_INCLUDE_DIR})
    target_include_directories(dyncapi_scorep_shared PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../selection)
    target_compile_definitions(dyncapi_scorep_shared PRIVATE "-DCAPI_SCOREP_SUPPORT")
    target_link_libraries(dyncapi_scorep_shared PRIVATE ${scorep_mgmt} ${scorep_measurement})
    #target_link_libraries(dyncapi_scorep_shared PRIVATE ${xray} ${xray-basic} ${xray-fdr})
    #target_link_options(dyncapi_scorep_shared PRIVATE -Wl,--whole-archive  ${xray} ${xray-basic} ${xray-fdr} -Wl,--no-whole-archive)
endif()


