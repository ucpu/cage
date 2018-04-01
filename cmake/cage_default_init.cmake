
macro(cage_default_init)
	cage_disable_source_build()
	cage_build_configuration()
	cage_output_directories(${CMAKE_BINARY_DIR}/result)
	cage_output_data(${CMAKE_BINARY_DIR}/result)
endmacro(cage_default_init)
