


set(TALP_INST_PASS_SOURCES
        TALPInstrumenter.cpp)

make_llvm_module(talp_inst
        "${TALP_INST_PASS_SOURCES}"
        INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/
        )

install(
        TARGETS talp_inst
        EXPORT ${TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)