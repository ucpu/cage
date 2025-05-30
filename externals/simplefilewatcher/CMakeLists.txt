message(STATUS "----------------------------------------------------------------")
message(STATUS "simplefilewatcher")
message(STATUS "----------------------------------------------------------------")

file(GLOB sources simplefilewatcher/source/*)
add_library(simplefilewatcher STATIC ${sources})
target_include_directories(simplefilewatcher PUBLIC simplefilewatcher/include)
target_compile_definitions(simplefilewatcher PRIVATE WIN32_LEAN_AND_MEAN VC_EXTRALEAN NOMINMAX)

