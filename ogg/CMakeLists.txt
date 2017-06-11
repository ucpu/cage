cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "ogg")
message(STATUS "----------------------------------------------------------------")

add_subdirectory(ogg)

get_property(OGG_INCLUDE_DIR TARGET ogg PROPERTY INCLUDE_DIRECTORIES)
set(OGG_LIBRARY $<TARGET_FILE:ogg>)

set(filename ${CMAKE_BINARY_DIR}/cmake/FindOGG.cmake)

file(WRITE ${filename} "set(OGG_FOUND 1)\n")
file(APPEND ${filename} "set(OGG_INCLUDE_DIR ${OGG_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(OGG_INCLUDE_DIRS ${OGG_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(OGG_LIBRARY ${OGG_LIBRARY})\n")
file(APPEND ${filename} "set(OGG_LIBRARIES ${OGG_LIBRARY})\n")

target_include_directories(ogg INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ogg/include)
