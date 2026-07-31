message(STATUS "----------------------------------------------------------------")
message(STATUS "abseil")
message(STATUS "----------------------------------------------------------------")

cage_heavily_optimized()

set(ABSL_PROPAGATE_CXX_STD OFF CACHE INTERNAL "ABSL_PROPAGATE_CXX_STD")
set(ABSL_USE_SYSTEM_INCLUDES ON CACHE INTERNAL "ABSL_USE_SYSTEM_INCLUDES")

add_subdirectory(abseil-cpp)
