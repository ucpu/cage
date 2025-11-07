message(STATUS "----------------------------------------------------------------")
message(STATUS "svector")
message(STATUS "----------------------------------------------------------------")

add_library(svector INTERFACE)
target_include_directories(svector INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/svector/include/ankerl>)
