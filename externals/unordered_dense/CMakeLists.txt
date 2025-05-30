message(STATUS "----------------------------------------------------------------")
message(STATUS "unordered_dense")
message(STATUS "----------------------------------------------------------------")

add_library(unordered_dense INTERFACE)
target_include_directories(unordered_dense INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/unordered_dense/include/ankerl>)
