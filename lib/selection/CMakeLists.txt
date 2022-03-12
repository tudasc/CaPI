
set(SOURCES CallGraph.cpp
        CaPI.cpp
        MetaCGReader.cpp
        DOTWriter.cpp
        Selector.cpp
        FunctionFilter.cpp
        SelectionSpecLoader.cpp
        SpecParser.cpp)

add_executable(capi ${SOURCES})
target_link_libraries(capi nlohmann_json::nlohmann_json)

set(UTILS_SOURCES CallGraph.cpp
        MetaCGReader.cpp
        DOTWriter.cpp
        Selector.cpp
        CaPIUtils.cpp)
add_executable(cgutils ${UTILS_SOURCES})
target_link_libraries(cgutils nlohmann_json::nlohmann_json)
