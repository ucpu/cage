cmake_minimum_required(VERSION 3.31)
project(cage LANGUAGES CXX C)

message(STATUS "----------------------------------------------------------------")
message(STATUS "cage start")
message(STATUS "----------------------------------------------------------------")

message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER_VERSION: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CMAKE_CXX_SIMULATE_ID: ${CMAKE_CXX_SIMULATE_ID}")
if(NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()
if(WIN32)
	message(STATUS "CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION: ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cage_all)

option(cage_faster_debug "enable some optimizations to improve performance in debug builds" ON)
option(cage_faster_release "enable more aggressive inlining in release builds" OFF)
option(cage_assert_in_release_enabled "enable runtime asserts in non-debug builds" OFF)
option(cage_validate_graphics "enable runtime validation in webgpu/dawn" ON)
option(cage_profiling_enabled "enable compiling-in instructions for profiling" ON)
option(cage_use_steam_sockets "include Game Networking Sockets library by Valve/Steam" OFF)

if(CMAKE_CURRENT_BINARY_DIR STREQUAL CMAKE_BINARY_DIR)
	cage_default_init() # standalone build
else()
	cage_fetch_cage_externals()
	cage_disable_source_build()
	cage_build_configuration()
endif()

add_subdirectory(externals EXCLUDE_FROM_ALL)
add_subdirectory(sources)
cage_ide_startup_project(cage-test-core)

message(STATUS "----------------------------------------------------------------")
message(STATUS "cage done")
message(STATUS "----------------------------------------------------------------")
