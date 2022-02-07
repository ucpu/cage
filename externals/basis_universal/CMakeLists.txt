cmake_minimum_required(VERSION 3.16)

message(STATUS "----------------------------------------------------------------")
message(STATUS "basis_universal")
message(STATUS "----------------------------------------------------------------")

file(GLOB_RECURSE basisu-sources "basis_universal/encoder/*" "basis_universal/transcoder/*")

add_library(basisu STATIC ${basisu-sources})

target_include_directories(basisu INTERFACE ".")
target_compile_definitions(basisu PRIVATE BASISU_SUPPORT_SSE=1 BASISD_SUPPORT_KTX2_ZSTD=1)
target_compile_definitions(basisu PUBLIC BASISU_NO_ITERATOR_DEBUG_LEVEL)
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(basisu PRIVATE -msse4.1)
endif()
