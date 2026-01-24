message(STATUS "----------------------------------------------------------------")
message(STATUS "quickhull")
message(STATUS "----------------------------------------------------------------")

# remove prints to cerr
cage_replace_in_file(
	"${CMAKE_CURRENT_LIST_DIR}/quickhull/QuickHull.cpp"
	"std::cerr << \"Failed to solve horizon edge.\" << std::endl;"
	"// cage: no printing please"
)

file(GLOB SOURCES quickhull/*.cpp quickhull/*.hpp quickhull/Structs/*)
add_library(quickhull STATIC ${SOURCES})
target_include_directories(quickhull INTERFACE .)
