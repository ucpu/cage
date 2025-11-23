message(STATUS "----------------------------------------------------------------")
message(STATUS "svector")
message(STATUS "----------------------------------------------------------------")

add_library(svector STATIC)
target_sources(svector PRIVATE dummy.cpp)
target_sources(svector PRIVATE "${CMAKE_CURRENT_LIST_DIR}/svector/svector.natvis")
target_include_directories(svector INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/svector/include/ankerl>)

