cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "ogg")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")

add_subdirectory(ogg)

set(OGG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ogg/include)
set(OGG_LIBRARY $<TARGET_LINKER_FILE:ogg>)
set(filename ${CMAKE_BINARY_DIR}/cmake/FindOGG.cmake)
file(WRITE ${filename} "set(OGG_FOUND 1)\n")
file(APPEND ${filename} "set(OGG_INCLUDE_DIR ${OGG_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(OGG_INCLUDE_DIRS ${OGG_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(OGG_LIBRARY ${OGG_LIBRARY})\n")
file(APPEND ${filename} "set(OGG_LIBRARIES ${OGG_LIBRARY})\n")

target_include_directories(ogg INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ogg/include)
