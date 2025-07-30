
set(SOURCES
        DriverUtils.cpp
        SCC.cpp
        Demangle.cpp
        DOTWriter.cpp
        FunctionFilter.cpp
        MeasurementConfigIO.cpp
        Preprocessor.cpp
        SpecParser.cpp
        SelectionSpecAST.cpp
        SelectorBuilder.cpp
        SelectorGraph.cpp
        StatementCountAnalysis.cpp
        TraversalHelper.cpp
        selectors/SelectorInstantiation.cpp
        selectors/BasicSelectors.cpp
        selectors/CommonCallerSelectorSCC.cpp)

add_library(capi_select SHARED ${SOURCES})
target_include_directories(capi_select
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_include_directories(capi_select
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(capi_select PUBLIC symbol_retriever)
target_link_libraries(capi_select PUBLIC metacg::metacg)
add_version_include(capi_select)

install(TARGETS capi_select
        EXPORT capiTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)




