cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "ogg")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")

add_subdirectory(ogg)

target_include_directories(ogg PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/ogg/include")
target_include_directories(ogg INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/ogg/include")

cage_gen_module(OGG $<TARGET_LINKER_FILE:ogg> "${CMAKE_CURRENT_SOURCE_DIR}/ogg/include;${CMAKE_CURRENT_BINARY_DIR}/ogg/include")
