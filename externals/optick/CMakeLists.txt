cmake_minimum_required(VERSION 3.16)

message(STATUS "----------------------------------------------------------------")
message(STATUS "optick")
message(STATUS "----------------------------------------------------------------")

option(OPTICK_ENABLED "Enable profiling with Optick" OFF)

option(OPTICK_INSTALL_TARGETS "Should optick be installed? Set to OFF if you use add_subdirectory to include Optick." OFF)

add_subdirectory(optick)
