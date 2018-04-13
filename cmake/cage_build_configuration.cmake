
function(cage_build_configuration)
	if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
		message(FATAL_ERROR "CMAKE_BUILD_TYPE needs to be set")
	endif()

	set(CMAKE_POSITION_INDEPENDENT_CODE ON PARENT_SCOPE)
	set(CMAKE_CXX_STANDARD 11 PARENT_SCOPE)

	if(CMAKE_COMPILER_IS_GNUCXX)
		# gcc: disable warnings about attributes
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-attributes" PARENT_SCOPE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes" PARENT_SCOPE)
	endif()

	if(MSVC)
		# msvc: multi-process compilation
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -MP" PARENT_SCOPE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MP" PARENT_SCOPE)
	endif()
endfunction(cage_build_configuration)

