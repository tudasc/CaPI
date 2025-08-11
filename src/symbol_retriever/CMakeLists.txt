
set(SYMBOL_RETRIEVER_SOURCES
        SymbolRetriever.cpp)

add_library(symbol_retriever STATIC ${SYMBOL_RETRIEVER_SOURCES})
target_include_directories(symbol_retriever
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

install(TARGETS symbol_retriever
        EXPORT capiTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)