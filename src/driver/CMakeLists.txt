add_executable(capi Driver.cpp)
target_include_directories(capi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(capi PRIVATE capi_select)

#target_include_directories(capi PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/graph/include>)

add_json(capi)
add_version_include(capi)

install(TARGETS capi EXPORT capiTargets RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
