message(STATUS "----------------------------------------------------------------")
message(STATUS "fastnoise")
message(STATUS "----------------------------------------------------------------")

add_library(fastnoise STATIC fastnoise/Cpp/FastNoiseLite.h dummy.cpp)
target_include_directories(fastnoise PUBLIC fastnoise/Cpp)

