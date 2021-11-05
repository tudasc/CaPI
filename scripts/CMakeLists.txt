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

function(configure_script input output)
    set(INSTCONTROL_BIN_DIR ${CMAKE_BINARY_DIR})
    set(INSTCONTROL_LIB_DIR ${CMAKE_BINARY_DIR}/lib)

    # To get execute permission: create run.sh in the binary dir, and copy it to scripts folder with permission
    make_executable(${input} ${output})
endfunction()

configure_script(clang-inst.in clang-inst)
configure_script(clang-inst++.in clang-inst++)
