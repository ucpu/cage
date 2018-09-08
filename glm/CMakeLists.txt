cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "glm")
message(STATUS "----------------------------------------------------------------")

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/glm")

cage_gen_module(glm $<TARGET_LINKER_FILE:glm> "")
