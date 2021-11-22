
function(cage_ide_sort_files target)
	get_property(files TARGET ${target} PROPERTY SOURCES)
	if(ARGV1)
		set(root "${ARGV1}")
	else()
		set(root "${CMAKE_CURRENT_SOURCE_DIR}")
	endif()
	source_group(TREE "${root}" FILES ${files})
endfunction(cage_ide_sort_files)

