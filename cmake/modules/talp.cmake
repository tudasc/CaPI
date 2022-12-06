option(ENABLE_TALP "Enable TALP user region instrumentation interface" ON)

if (ENABLE_TALP)
    set(DLB_DIR "/usr/local" CACHE STRING "Path to the DLB install directory (needed for TALP)")
    set(DLB_INCLUDE_DIR "${DLB_DIR}/include")

    find_library(dlb_mpi_lib  NAMES dlb_mpi HINTS "${DLB_DIR}/lib"  REQUIRED)

endif()