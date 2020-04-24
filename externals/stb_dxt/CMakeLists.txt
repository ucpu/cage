
message(STATUS "----------------------------------------------------------------")
message(STATUS "stb_dxt")
message(STATUS "----------------------------------------------------------------")

add_library(stb_dxt STATIC stb_dxt.c stb_dxt.h)
target_include_directories(stb_dxt PUBLIC .)
