option(ENABLE_NESMIK "Enable neSmiK interface with XRay" OFF)

if (ENABLE_NESMIK)
    #set(NESMIK_DIR "/usr/local" CACHE STRING "Path to the neSmiK install directory")
    #set(DLB_INCLUDE_DIR "${DLB_DIR}/include")

    find_package(nesmik REQUIRED)
    get_target_property(NESMIK_LIB_PATH nesmik::nesmik LOCATION)

    message(STATUS "neSmiK library: ${NESMIK_LIB_PATH}")
endif()