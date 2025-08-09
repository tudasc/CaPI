option(ENABLE_FLIP "Enable selectors based on FLIP profiles" OFF)

if (ENABLE_FLIP)
    find_package(Flip REQUIRED)

    get_target_property(FLIP_INCLUDE_DIR flip::FLIP_counts INTERFACE_INCLUDE_DIRECTORIES)

    message(STATUS "FLIP MetaCG metadata include: ${FLIP_INCLUDE_DIR}")
endif()
