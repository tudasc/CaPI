
set(SOURCES
        CaPI.cpp
        SCC.cpp
        MetaCGReader.cpp.backup
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

add_executable(capi ${SOURCES})
target_include_directories(capi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/lib/symbol_retriever ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(capi PRIVATE symbol_retriever)
target_link_libraries(capi PRIVATE metacg::metacg)
#target_include_directories(capi PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/graph/include>)

add_json(capi)
add_version_include(capi)

install(TARGETS capi RUNTIME DESTINATION bin)
