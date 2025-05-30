message(STATUS "----------------------------------------------------------------")
message(STATUS "stb")
message(STATUS "----------------------------------------------------------------")

file(GLOB stb-sources "stb/*.h")
add_library(stb STATIC stb.c ${stb-sources})
target_include_directories(stb PUBLIC stb)
