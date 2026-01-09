set(NESMIK_INST_PASS_SOURCES
        NeSmiKPass.cpp)

make_llvm_module(nesmik_inst
        "${NESMIK_INST_PASS_SOURCES}"
        LINK_LIBS
        nlohmann_json::nlohmann_json
        DEPENDS
        INCLUDE_DIRS
        ..
        ../..
        )

install(
        TARGETS nesmik_inst
        EXPORT ${TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)