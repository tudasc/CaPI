
set(SYMBOL_RETRIEVER_SOURCES
        SymbolRetriever.cpp)

add_library(symbol_retriever STATIC ${SYMBOL_RETRIEVER_SOURCES})
target_include_directories(symbol_retriever PRIVATE ${PROJECT_SOURCE_DIR}/lib)
