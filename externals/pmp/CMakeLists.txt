cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "pmp")
message(STATUS "----------------------------------------------------------------")

project(pmp CXX C)

file(GLOB SOURCES pmp/src/pmp/*.cpp pmp/src/pmp/algorithms/*.cpp)
file(GLOB HEADERS pmp/src/pmp/*.h pmp/src/pmp/algorithms/*.h)

add_library(pmp STATIC ${SOURCES} ${HEADERS})

target_sources(pmp PRIVATE pmp/external/rply/rply.h pmp/external/rply/rply.c pmp/external/rply/rplyfile.h)
target_include_directories(pmp PRIVATE pmp/external/rply)

target_compile_definitions(pmp PUBLIC _USE_MATH_DEFINES EIGEN_MPL2_ONLY)
target_include_directories(pmp PUBLIC pmp/external/eigen)
target_include_directories(pmp PUBLIC pmp/src)
