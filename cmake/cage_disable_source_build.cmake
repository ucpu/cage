
macro(cage_disable_source_build)
	set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
	set(CMAKE_DISABLE_SOURCE_CHANGES ON)
	if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
		message(FATAL_ERROR "In-source build is disabled. Remove the already generated files and start again from dedicated build directory.")
	ENDIF()
endmacro(cage_disable_source_build)
