cmake_minimum_required(VERSION 3.20)

message(STATUS "----------------------------------------------------------------")
message(STATUS "pmp")
message(STATUS "----------------------------------------------------------------")

project(pmp CXX C)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SOURCES pmp/src/pmp/*.cpp pmp/src/pmp/algorithms/*.cpp)
file(GLOB HEADERS pmp/src/pmp/*.h pmp/src/pmp/algorithms/*.h)

add_library(pmp STATIC ${SOURCES} ${HEADERS})

target_compile_definitions(pmp PUBLIC _USE_MATH_DEFINES EIGEN_MPL2_ONLY)
target_include_directories(pmp PUBLIC pmp/external/eigen)
target_include_directories(pmp PUBLIC pmp/src)
