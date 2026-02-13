message(STATUS "----------------------------------------------------------------")
message(STATUS "mimalloc")
message(STATUS "----------------------------------------------------------------")

cage_heavily_optimized()

set(MI_USE_CXX OFF CACHE INTERNAL "use c++ compiler for mimalloc")
set(MI_OVERRIDE ${WIN32} CACHE INTERNAL "Override the standard malloc interface (i.e. define entry points for 'malloc', 'free', etc)")
set(MI_BUILD_SHARED ${WIN32} CACHE INTERNAL "Build shared library")
set(MI_BUILD_STATIC (NOT ${WIN32}) CACHE INTERNAL "Build static library")
set(MI_BUILD_OBJECT OFF CACHE INTERNAL "Build object library")
set(MI_BUILD_TESTS OFF CACHE INTERNAL "Build test executables")

add_subdirectory(mimalloc)

if(WIN32)
	add_library(cage-mimalloc ALIAS mimalloc)
else()
	add_library(cage-mimalloc ALIAS mimalloc-static)
endif()
