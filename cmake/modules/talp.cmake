option(ENABLE_TALP "Enable TALP user region instrumentation interface" ON)

if (ENABLE_TALP)

    find_package(DLB REQUIRED)  # Try to find an existing installation

    get_target_property(DLB_INCLUDE_DIR DLB::DLB INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(DLB_LIBRARY DLB::DLB IMPORTED_LOCATION_RELEASE)

    get_filename_component(DLB_LIB_DIR ${DLB_LIBRARY} DIRECTORY)

    message(STATUS "DLB library: ${DLB_LIBRARY}")

endif()