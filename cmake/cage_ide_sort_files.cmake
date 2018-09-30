
function(cage_ide_sort_files target)
	if(${CMAKE_VERSION} VERSION_GREATER 3.8)
		get_property(files TARGET ${target} PROPERTY SOURCES)
		source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${files})
	endif()
endfunction(cage_ide_sort_files)

