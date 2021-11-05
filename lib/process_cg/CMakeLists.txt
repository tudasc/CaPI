
set(SOURCES CallGraph.cpp
        PrepareCallgraph.cpp
        MetaCGReader.cpp
        DOTWriter.cpp
        Selector.cpp)

add_executable(preparecg ${SOURCES})
