
function(cage_ide_working_dir target directory)
	if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.8)
		set_target_properties(${target} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${directory})
	endif()
endfunction(cage_ide_working_dir)

function(cage_ide_working_dir_in_place target)
	if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.13)
		cage_ide_working_dir(${target} "$<TARGET_FILE_DIR:${target}>")
	else()
		cage_ide_working_dir(${target} "$(OutDir)")
	endif()
endfunction(cage_ide_working_dir_in_place)
