cmake_minimum_required(VERSION 3.20)

message(STATUS "----------------------------------------------------------------")
message(STATUS "pmp")
message(STATUS "----------------------------------------------------------------")

file(GLOB SOURCES pmp/src/pmp/*.cpp pmp/src/pmp/algorithms/*.cpp)
file(GLOB HEADERS pmp/src/pmp/*.h pmp/src/pmp/algorithms/*.h)

add_library(pmp STATIC ${SOURCES} ${HEADERS})

target_compile_definitions(pmp PUBLIC _USE_MATH_DEFINES EIGEN_MPL2_ONLY)
target_include_directories(pmp PUBLIC pmp/external/eigen-3.4.0)
target_include_directories(pmp PUBLIC pmp/src)

if(MSVC)
	target_compile_options(pmp PRIVATE /wd4267)
	target_compile_options(pmp PRIVATE /wd4244)
	target_compile_options(pmp PRIVATE /wd4305)
endif()
