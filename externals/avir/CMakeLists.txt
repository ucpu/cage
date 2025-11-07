message(STATUS "----------------------------------------------------------------")
message(STATUS "avir")
message(STATUS "----------------------------------------------------------------")

cage_heavily_optimized()

add_library(avir STATIC avir/avir.h avir/avir_dil.h avir/avir_float4_sse.h avir/avir_float8_avx.h avir/lancir.h dummy.cpp)

target_include_directories(avir PUBLIC .)
