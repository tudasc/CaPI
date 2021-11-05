option(USE_EXTERNAL_JSON "Use external JSON library" OFF)

if (USE_EXTERNAL_JSON)
    find_package(nlohmann_json REQUIRED)
    message(STATUS "Found nlohmann_json ${nlohmann_json_PACKAGE_VERSION}")
else()
    FetchContent_Declare(json
            GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent.git
            GIT_TAG v3.10.4)

    FetchContent_GetProperties(json)
    if(NOT json_POPULATED)
        FetchContent_Populate(json)
        add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()

endif()
