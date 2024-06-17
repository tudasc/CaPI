
set(SOURCES CallGraph.cpp
        CaPI.cpp
        SCC.cpp
        MetaCGReader.cpp
        DOTWriter.cpp
        FunctionFilter.cpp
        Preprocessor.cpp
        SpecParser.cpp
        SelectionSpecAST.cpp
        SelectorBuilder.cpp
        SelectorGraph.cpp
        selectors/SelectorInstantiation.cpp
        selectors/BasicSelectors.cpp
        selectors/CommonCallerSelectorSCC.cpp)

add_executable(capi ${SOURCES})
target_include_directories(capi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/lib/symbol_retriever ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(capi PRIVATE symbol_retriever)
add_json(capi)
add_version_include(capi)

install(TARGETS capi RUNTIME DESTINATION bin)
