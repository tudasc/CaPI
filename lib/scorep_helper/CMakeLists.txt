
set(SCOREP_PATH "" CACHE PATH "Path to ScoreP installation")

find_library(scorep_mgmt scorep_adapter_compiler_mgmt HINTS ${SCOREP_PATH}/lib REQUIRED)
find_library(scorep_measurement scorep_measurement HINTS ${SCOREP_PATH}/lib REQUIRED)

message(STATUS "ScoreP libs: ${scorep_mgmt} ${scorep_measurement}")

add_library(scorep_helper_static STATIC ScorePHelper.cpp)
target_link_libraries(scorep_helper_static PRIVATE ${scorep_mgmt} ${scorep_measurement})

add_library(scorep_helper SHARED ScorePHelper.cpp)
target_link_libraries(scorep_helper PRIVATE ${scorep_mgmt} ${scorep_measurement})

