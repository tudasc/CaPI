
set(SOURCES CallGraph.cpp
        CaPI.cpp
        MetaCGReader.cpp
        DOTWriter.cpp
        FunctionFilter.cpp
        Preprocessor.cpp
        SpecParser.cpp
        SelectionSpecAST.cpp
        SelectorBuilder.cpp
        SelectorGraph.cpp
        selectors/SelectorInstantiation.cpp
        selectors/BasicSelectors.cpp)

add_executable(capi ${SOURCES})
target_include_directories(capi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_json(capi)

set(UTILS_SOURCES CallGraph.cpp
        MetaCGReader.cpp
        DOTWriter.cpp
        CaPIUtils.cpp)
add_executable(cgutils ${UTILS_SOURCES})
add_json(cgutils)