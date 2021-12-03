cmake_minimum_required(VERSION 3.16)

message(STATUS "----------------------------------------------------------------")
message(STATUS "astc")
message(STATUS "----------------------------------------------------------------")

option(ISA_AVX2 "Enable builds for AVX2 SIMD" ON)
option(ISA_SSE41 "Enable builds for SSE4.1 SIMD" OFF)
option(ISA_SSE2 "Enable builds for SSE2 SIMD" OFF)
option(ISA_NEON "Enable builds for NEON SIMD" OFF)
option(ISA_NONE "Enable builds for no SIMD" OFF)
option(ISA_NATIVE "Enable builds for native SIMD" OFF)
option(DECOMPRESSOR "Enable builds for decompression only" OFF)
option(DIAGNOSTICS "Enable builds for diagnostic trace" OFF)
option(UNITTEST "Enable builds for unit tests" OFF)
option(CLI "Enable build of CLI" OFF)

add_subdirectory(astc)
