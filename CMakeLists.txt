cmake_minimum_required(VERSION 3.2)

project(cage)

# disable source changes
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)

# compile options
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 11)

# allow grouping of projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# add subprojects
add_subdirectory(externals EXCLUDE_FROM_ALL)
add_subdirectory(core)
add_subdirectory(client)
add_subdirectory(data)

message(STATUS "----------------------------------------------------------------")
message(STATUS "cage done")
message(STATUS "----------------------------------------------------------------")

# some helper macros
macro(print_all_variables)
	message(STATUS "----------------------------------------------------------------")
	message(STATUS "print all variables begin")
	message(STATUS "----------------------------------------------------------------")
    get_cmake_property(variableNames VARIABLES)
    foreach (variableName ${variableNames})
        message(STATUS "${variableName} = ${${variableName}}")
    endforeach()
	message(STATUS "----------------------------------------------------------------")
	message(STATUS "print all variables end")
	message(STATUS "----------------------------------------------------------------")
endmacro()
