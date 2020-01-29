
function(cage_ide_sort_files target)
	get_property(files TARGET ${target} PROPERTY SOURCES)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${files})
endfunction(cage_ide_sort_files)

