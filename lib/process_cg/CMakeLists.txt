
set(SOURCES CallGraph.cpp
        PrepareCallgraph.cpp
        MetaCGReader.cpp
        DOTWriter.cpp
        Selector.cpp)

add_executable(preparecg ${SOURCES})
target_link_libraries(preparecg nlohmann_json::nlohmann_json)

