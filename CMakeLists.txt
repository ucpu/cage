cmake_minimum_required(VERSION 3.10)
project(cage LANGUAGES CXX C)

message(STATUS "----------------------------------------------------------------")
message(STATUS "cage start")
message(STATUS "----------------------------------------------------------------")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cage_all)

if(CMAKE_CURRENT_BINARY_DIR STREQUAL CMAKE_BINARY_DIR)
	cage_default_init() # standalone build
else()
	cage_disable_source_build()
	cage_build_configuration()
endif()

add_subdirectory(externals EXCLUDE_FROM_ALL)
add_subdirectory(sources)

message(STATUS "----------------------------------------------------------------")
message(STATUS "cage done")
message(STATUS "----------------------------------------------------------------")
