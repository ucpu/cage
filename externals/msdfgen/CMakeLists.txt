message(STATUS "----------------------------------------------------------------")
message(STATUS "msdfgen")
message(STATUS "----------------------------------------------------------------")

file(GLOB_RECURSE msdfgen-sources "msdfgen/msdfgen.h" "msdfgen/core/*" "msdfgen/ext/import-font.*")
add_library(lib_msdfgen ${msdfgen-sources})
target_compile_definitions(lib_msdfgen PUBLIC MSDFGEN_USE_CPP11 MSDFGEN_PUBLIC=)
target_include_directories(lib_msdfgen INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_msdfgen PUBLIC freetype)
