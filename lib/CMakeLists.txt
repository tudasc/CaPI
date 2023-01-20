
set(PASS_SOURCES
        ReadMetaCG.cpp
        MetaCGReader.cpp)

make_llvm_module(readmetacg
        "${PASS_SOURCES}"
        LINK_LIBS
            nlohmann_json::nlohmann_json
        DEPENDS
        INCLUDE_DIRS
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${PROJECT_SOURCE_DIR}/lib/
        )


install(
        TARGETS readmetacg
        EXPORT ${TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)

add_subdirectory(passes)
add_subdirectory(selection)
add_subdirectory(calltree_verifier)
add_subdirectory(symbol_retriever)

if (ENABLE_XRAY)
    add_subdirectory(xray)
    add_subdirectory(dyncapi)
endif()