message(STATUS "----------------------------------------------------------------")
message(STATUS "harfbuzz")
message(STATUS "----------------------------------------------------------------")

set(SKIP_INSTALL_ALL ON)
add_subdirectory(harfbuzz)
target_include_directories(harfbuzz PUBLIC harfbuzz/src)
