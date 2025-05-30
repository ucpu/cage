message(STATUS "----------------------------------------------------------------")
message(STATUS "freetype")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")
set(SKIP_INSTALL_ALL ON CACHE INTERNAL "SKIP_INSTALL_ALL")

add_subdirectory(freetype)

target_include_directories(freetype INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/freetype/include")
target_include_directories(freetype INTERFACE "${CMAKE_CURRENT_BUILD_DIR}/freetype/include")

cage_gen_module(Freetype $<TARGET_LINKER_FILE:freetype> $<TARGET_PROPERTY:freetype,INTERFACE_INCLUDE_DIRECTORIES>)

