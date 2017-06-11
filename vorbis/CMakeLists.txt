cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "vorbis")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")

find_package(OGG)

add_subdirectory(vorbis)

target_include_directories(vorbisenc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vorbis/include)
target_include_directories(vorbisfile INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vorbis/include)
