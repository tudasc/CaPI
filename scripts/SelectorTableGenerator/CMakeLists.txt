add_executable(generator SelectorTableGenerator.cpp)
target_link_libraries(generator
    PRIVATE
    capi_select
)
