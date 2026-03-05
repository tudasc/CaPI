include(GNUInstallDirs)

macro(make_executable input output)
    # Executable permission: In CMAKE 3.19 we can simply use file(CHMOD) instead of file(copy) workaround
    configure_file(${input} ${CMAKE_CURRENT_BINARY_DIR}/tmp/${output} @ONLY)
    file(
            COPY ${CMAKE_CURRENT_BINARY_DIR}/tmp/${output}
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
            FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
    file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/tmp)
endmacro()

function(configure_script input output mode)
    if(mode STREQUAL "BUILD")
        set(CAPI_BIN_DIR "${CMAKE_BINARY_DIR}")
        set(CAPI_LIB_DIR "${CMAKE_BINARY_DIR}/src/")
        set(CAPI_INST_PASS_DIR "${CAPI_LIB_DIR}/passes/capi_inst")
        set(CAPI_NESMIK_PASS_DIR "${CAPI_LIB_DIR}/passes/nesmik")
        set(CAPI_TALP_PASS_DIR "${CAPI_LIB_DIR}/passes/talp")
        set(CAPI_XRAY_LIB_DIR "${CAPI_LIB_DIR}/xray")
        set(CAPI_SELECT_LIB_DIR "${CMAKE_BINARY_DIR}/src/selection")
        set(CAPI_NESMIK_MERGE_LIB_DIR "${CMAKE_BINARY_DIR}/src/nesmik_merger")
    elseif(mode STREQUAL "INSTALL")
        set(CAPI_BIN_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
        set(CAPI_LIB_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
        set(CAPI_INST_PASS_DIR "${CAPI_LIB_DIR}")
        set(CAPI_NESMIK_PASS_DIR "${CAPI_LIB_DIR}")
        set(CAPI_TALP_PASS_DIR "${CAPI_LIB_DIR}")
        set(CAPI_XRAY_LIB_DIR "${CAPI_LIB_DIR}")
        set(CAPI_SELECT_LIB_DIR "${CAPI_LIB_DIR}")
        set(CAPI_NESMIK_MERGE_LIB_DIR "${CAPI_LIB_DIR}")
    else()
        message(FATAL_ERROR "Unknown configure_script mode: ${mode}")
    endif()

    if ("${ENABLE_INST_PASSES}")
        set(INST_PASSES_AVAILABLE "1")
    else()
        set(INST_PASSES_AVAILABLE "0")
    endif()
    set(LLVM_CONFIG "${LLVM_BINARY_DIR}/bin/llvm-config")
    set(SCOREP_CONFIG "${SCOREP_DIR}/bin/scorep-config")
    set(CC_PATH "${LLVM_BINARY_DIR}/bin/clang")
    set(CXX_PATH "${LLVM_BINARY_DIR}/bin/clang++")

    make_executable(${input} ${output})
endfunction()

# Build tree version
configure_script(capicc.in capicc BUILD)

# Install tree version
configure_script(capicc.in capicc.install INSTALL)
install(
        PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/capicc.install
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        RENAME capicc
)

# Wrapper generator
make_executable(make_capicc_wrapper.sh.in make_capicc_wrapper.sh)
install(
        PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/make_capicc_wrapper.sh
        DESTINATION ${CMAKE_INSTALL_BINDIR}
)


