
set(SCOREP_PATH "" CACHE PATH "Path to ScoreP installation")

find_library(scorep_mgmt_lib scorep_adapter_compiler_mgmt PATHS ${SCOREP_PATH}/lib REQUIRED)


add_library(scorep_helper_static STATIC ScorePHelper.cpp)
target_link_libraries(scorep_helper_static PRIVATE ${scorep_mgmt_lib})

add_library(scorep_helper SHARED ScorePHelper.cpp)
target_link_libraries(scorep_helper PRIVATE ${scorep_mgmt_lib})

