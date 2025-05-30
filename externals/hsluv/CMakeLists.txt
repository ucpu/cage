message(STATUS "----------------------------------------------------------------")
message(STATUS "hsluv")
message(STATUS "----------------------------------------------------------------")

add_library(hsluv STATIC hsluv.c hsluv.h)
target_include_directories(hsluv PUBLIC .)
