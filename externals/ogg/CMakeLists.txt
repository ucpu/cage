cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "ogg")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")
set(INSTALL_DOCS OFF CACHE INTERNAL "Install documentation")
set(INSTALL_PKG_CONFIG_MODULE OFF CACHE INTERNAL "Install ogg.pc file")
set(INSTALL_CMAKE_PACKAGE_MODULE OFF CACHE INTERNAL "Install CMake package configiguration module")

add_subdirectory(ogg)

target_include_directories(ogg PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/ogg/include")
target_include_directories(ogg INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/ogg/include")

cage_gen_module(OGG $<TARGET_LINKER_FILE:ogg> $<TARGET_PROPERTY:ogg,INTERFACE_INCLUDE_DIRECTORIES>)
