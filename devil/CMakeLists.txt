cmake_minimum_required(VERSION 3.10)

message(STATUS "----------------------------------------------------------------")
message(STATUS "devil")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS ON CACHE INTERNAL "BUILD_SHARED_LIBS")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/devil/DevIL/cmake/Modules")

# disable some libraries
option(IL_JASPER_LIB "Custom location for Jasper" ON)
option(IL_NO_JP2 "Disable JP2 support (libjasper)" ON)
option(IL_SQUISH_LIB "Custom location for libSquish" ON)
option(IL_USE_DXTC_SQUISH "Use libsquish for DXTC support" OFF)
option(IL_LCMS2_LIB "Custom location for LCMS2" ON)
option(IL_NO_LCMS "Disable LCMS support (Little CMS)" ON)

add_subdirectory(devil/DevIL/src-IL)
add_subdirectory(devil/DevIL/src-ILU)

target_include_directories(IL INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/devil/DevIL/include")

cage_gen_module(IL IL "")
