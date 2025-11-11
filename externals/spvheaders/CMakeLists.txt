message(STATUS "----------------------------------------------------------------")
message(STATUS "spvheaders")
message(STATUS "----------------------------------------------------------------")

set(SPIRV_HEADERS_ENABLE_TESTS OFF CACHE INTERNAL "")
set(SPIRV_HEADERS_ENABLE_INSTALL OFF CACHE INTERNAL "")

add_subdirectory(spvheaders)
