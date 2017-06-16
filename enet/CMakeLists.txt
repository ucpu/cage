cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "enet")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")

add_subdirectory(enet)

target_include_directories(enet INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/enet/include)

