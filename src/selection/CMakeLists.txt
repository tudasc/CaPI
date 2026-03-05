
set(SOURCES
        DriverUtils.cpp
        SCC.cpp
        CallGraphExtractor.cpp
        Demangle.cpp
        DOTWriter.cpp
        FunctionFilter.cpp
        MeasurementConfig.cpp
        MeasurementConfigIO.cpp
        Preprocessor.cpp
        QueryParser.cpp
        SelectionQueryAST.cpp
        SelectorBuilder.cpp
        SelectorGraph.cpp
        StatementCountAnalysis.cpp
        TraversalHelper.cpp
        selectors/SelectorInstantiation.cpp
        selectors/BasicSelectors.cpp
        selectors/CommonCallerSelectorSCC.cpp)

add_library(capi_select SHARED ${SOURCES})

target_link_libraries(capi_select PUBLIC
        LLVMObject
        LLVMSupport
)
target_link_libraries(capi_select PUBLIC symbol_retriever)
target_link_libraries(capi_select PUBLIC metacg::metacg)
target_link_libraries(capi_select PUBLIC cage)
add_version_include(capi_select)

if (ENABLE_FLIP)
        target_compile_definitions(capi_select PRIVATE CAPI_ENABLE_FLIP)
        target_link_libraries(capi_select PRIVATE flip::FLIP_counts)
        target_sources(capi_select PRIVATE selectors/FlipMetricSelector.cpp)
endif()


target_include_directories(capi_select
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_include_directories(capi_select
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS capi_select
        EXPORT capiTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)




