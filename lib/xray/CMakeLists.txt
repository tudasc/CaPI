
add_library(dyncapi STATIC XRayRuntime.cpp ../selection/FunctionFilter.cpp)

# TODO: Is there a better way to determine this include path?
set(XRAY_INCLUDE_DIR ${LLVM_LIBRARY_DIR}/clang/${LLVM_VERSION}/include)
#message("XRay include dir:  ${XRAY_INCLUDE_DIR}")

target_include_directories(dyncapi PRIVATE ${XRAY_INCLUDE_DIR})