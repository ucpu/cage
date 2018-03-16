
function(cage_ide_working_dir target directory)
	if(${CMAKE_VERSION} VERSION_GREATER 3.8)
		set_target_properties(${target} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${directory})
	endif()
endfunction(cage_ide_working_dir)

function(cage_ide_working_dir_in_place target)
	cage_ide_working_dir(${target} "$(OutDir)")
endfunction(cage_ide_working_dir_in_place)
