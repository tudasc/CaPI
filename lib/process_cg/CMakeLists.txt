
set(SOURCES CallGraph.cpp
        PrepareCallgraph.cpp
        MetaCGReader.cpp
        DOTWriter.cpp
        Selector.cpp
        FunctionFilter.cpp
        SelectionSpecLoader.cpp)

add_executable(preparecg ${SOURCES})
target_link_libraries(preparecg nlohmann_json::nlohmann_json)

set(UTILS_SOURCES CallGraph.cpp
        MetaCGReader.cpp
        DOTWriter.cpp
        Selector.cpp
        CGUtils.cpp)
add_executable(cgutils ${UTILS_SOURCES})
target_link_libraries(cgutils nlohmann_json::nlohmann_json)
