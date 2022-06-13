

set(CAPI_INST_PASS_SOURCES
        CaPIInstrumenter.cpp)


make_llvm_module(capi_inst
        "${CAPI_INST_PASS_SOURCES}"
        LINK_LIBS
        nlohmann_json::nlohmann_json
        DEPENDS
        INCLUDE_DIRS
        ..
        ../..
        )

#set(TALP_INST_PASS_SOURCES
#        TALPInstrumenter.cpp)

#make_llvm_module(talpinst
#        "${TALP_INST_PASS_SOURCES}"
#        INCLUDE_DIRS
#        ${CMAKE_CURRENT_SOURCE_DIR}
#        ${PROJECT_SOURCE_DIR}/lib/
#        )

install(
        TARGETS capi_inst
        EXPORT ${TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)