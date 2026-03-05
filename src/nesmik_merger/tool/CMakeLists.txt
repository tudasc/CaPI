add_executable(ntcgm NesmikMergeTool.cpp)
target_link_libraries(ntcgm PRIVATE merge_nesmik)
target_link_libraries(ntcgm PRIVATE cage_metadata)
add_cxxopts(ntcgm)
add_metacg(ntcgm)

install(TARGETS ntcgm EXPORT capiTargets RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
