cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "freetype")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")
set(SKIP_INSTALL_ALL ON)
set(FREETYPE_NO_DIST TRUE)
set(WITH_HarfBuzz OFF)

add_subdirectory(freetype)

target_include_directories(freetype INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/freetype/include")

