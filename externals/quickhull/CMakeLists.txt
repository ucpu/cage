message(STATUS "----------------------------------------------------------------")
message(STATUS "quickhull")
message(STATUS "----------------------------------------------------------------")


# remove prints to cerr
set(quickhull_path "${CMAKE_CURRENT_LIST_DIR}/quickhull/QuickHull.cpp")
file(READ "${quickhull_path}" quickhull_content)
string(REPLACE "std::cerr << \"Failed to solve horizon edge.\" << std::endl;" "// cage: no printing please" quickhull_content "${quickhull_content}")
set(CMAKE_DISABLE_SOURCE_CHANGES OFF)
file(CONFIGURE OUTPUT "${quickhull_path}" CONTENT "${quickhull_content}" @ONLY)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)


file(GLOB SOURCES quickhull/*.cpp quickhull/*.hpp quickhull/Structs/*)

add_library(quickhull STATIC ${SOURCES})

target_include_directories(quickhull INTERFACE .)
