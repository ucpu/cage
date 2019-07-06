cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "optick")
message(STATUS "----------------------------------------------------------------")

add_subdirectory(optick)

#option(CAGE_USE_OPTICK_PROFILER "integrate Optick library (formerly Brofiler) to cage" ON)
#if (CAGE_USE_OPTICK_PROFILER)
#	target_compile_definitions(OptickCore PUBLIC USE_OPTICK=1)
#	target_compile_definitions(OptickCore PUBLIC OPTICK_ENABLE_GPU=0)
#else()
#	target_compile_definitions(OptickCore PUBLIC USE_OPTICK=0)
#endif()

