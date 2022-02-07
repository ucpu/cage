cmake_minimum_required(VERSION 3.16)

message(STATUS "----------------------------------------------------------------")
message(STATUS "avir")
message(STATUS "----------------------------------------------------------------")

add_library(avir STATIC avir/avir.h avir/avir_dil.h avir/avir_float4_sse.h avir/avir_float8_avx.h avir/lancir.h dummy.cpp)

target_include_directories(avir PUBLIC .)
