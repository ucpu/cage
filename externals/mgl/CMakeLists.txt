message(STATUS "----------------------------------------------------------------")
message(STATUS "mgl")
message(STATUS "----------------------------------------------------------------")

if(NOT APPLE)
	message(STATUS "mgl is used on macos only - skipping")
	return()
endif()

add_subdirectory(mgl)

add_library(glfw-imp STATIC IMPORTED)
set_target_properties(glfw-imp PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/mgl/external/glfw/build/src/libglfw3.a")

add_library(glfw INTERFACE)
target_link_libraries(glfw INTERFACE glfw-imp mgl)
target_include_directories(glfw INTERFACE "mgl/external/glfw/include")

add_dependencies(glfw mgl)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "mgl/MGL/include")
