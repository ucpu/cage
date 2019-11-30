
function(cage_build_configuration)
	if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
		message(FATAL_ERROR "CMAKE_BUILD_TYPE needs to be set")
	endif()

	set(CMAKE_POSITION_INDEPENDENT_CODE ON PARENT_SCOPE)
	set(CMAKE_C_STANDARD 11)
	set(CMAKE_CXX_STANDARD 14)

	if(CMAKE_COMPILER_IS_GNUCXX)
		# disable warnings about attributes
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-attributes")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")
	endif()

	if(MSVC)
		# multi-process compilation
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
		# conformance to standard
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
		# enforce specific c++ standard
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++${CMAKE_CXX_STANDARD}")
	endif()

	set(CMAKE_C_STANDARD ${CMAKE_C_STANDARD} PARENT_SCOPE)
	set(CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD} PARENT_SCOPE)
	set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} PARENT_SCOPE)
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} PARENT_SCOPE)
endfunction(cage_build_configuration)

