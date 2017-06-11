cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "assimp")
message(STATUS "----------------------------------------------------------------")

option(BUILD_SHARED_LIBS "Build package with shared libraries." OFF)
option(ASSIMP_NO_EXPORT "Disable Assimp's export functionality." ON)
option(ASSIMP_BUILD_ASSIMP_TOOLS "If the supplementary tools for Assimp are built in addition to the library." OFF)
option(ASSIMP_BUILD_TESTS "If the test suite for Assimp is built in addition to the library." OFF)

find_package(ZLIB)

if(MSVC)
	list(APPEND CMAKE_CXX_FLAGS "/bigobj")
endif()

add_subdirectory(assimp)

target_include_directories(assimp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/assimp/include)
target_include_directories(assimp INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/assimp/include)
