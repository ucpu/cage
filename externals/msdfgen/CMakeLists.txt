cmake_minimum_required(VERSION 3.10)

message(STATUS "----------------------------------------------------------------")
message(STATUS "msdfgen")
message(STATUS "----------------------------------------------------------------")

add_subdirectory(msdfgen)

target_compile_definitions(lib_msdfgen PUBLIC MSDFGEN_NO_PRAGMA_LIB)
target_include_directories(lib_msdfgen INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/msdfgen)

