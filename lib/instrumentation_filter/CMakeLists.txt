

set(INSTFILTER_PASS_SOURCES
        ApplyInstrumentationFilter.cpp)


make_llvm_module(instfilter
        "${INSTFILTER_PASS_SOURCES}"
        LINK_LIBS
        nlohmann_json::nlohmann_json
        DEPENDS
        INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/lib/
        )

install(
        TARGETS instfilter
        EXPORT ${TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)