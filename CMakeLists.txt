cmake_minimum_required(VERSION 3.2)

project(cage)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_CXX_STANDARD 11)

add_subdirectory(externals)
add_subdirectory(core)
add_subdirectory(client)
