message(STATUS "----------------------------------------------------------------")
message(STATUS "glad")
message(STATUS "----------------------------------------------------------------")

unset(CMAKE_C_STANDARD)
unset(CMAKE_CXX_STANDARD)

add_library(glad STATIC glad/src/glad.c)
target_include_directories(glad PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/glad/include>)
if(MSVC)
	target_compile_definitions(glad PRIVATE "GLAPI=__declspec(dllexport)")
else()
	target_compile_definitions(glad PRIVATE "GLAPI=__attribute__((visibility(\"default\")))")
endif()
