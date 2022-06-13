option(ENABLE_TALP "Enable TALP user region instrumentation interface" ON)

if (ENABLE_TALP)
    set(DLB_LIB_DIR "/usr/local" CACHE STRING "Path to the DLB libraries (needed for TALP)")
endif()