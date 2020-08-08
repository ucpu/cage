
set(cage_assets_current_list_dir "${CMAKE_CURRENT_LIST_DIR}")
set(cage_assets_destination_path "${CMAKE_BINARY_DIR}/result/assets.zip" CACHE FILEPATH "Where to save the final archive with all assets.")
set(cage_assets_intermediate_path "${CMAKE_BINARY_DIR}/result/data" CACHE PATH "Directory for assets processing and database.")

function(cage_assets_generate_config)
	set(input "${cage_assets_intermediate_path}/data")
	set(schemes "${cage_assets_intermediate_path}/schemes")
	set(output "${cage_assets_destination_path}")
	set(intermediate "${cage_assets_intermediate_path}/processing")
	set(database "${cage_assets_intermediate_path}")
	foreach(conf IN ITEMS ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
		string(TOUPPER ${conf} conf_upper)
		configure_file("${cage_assets_current_list_dir}/cage-asset-database.in.ini" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${conf_upper}}/cage-asset-database.ini")
	endforeach(conf)
endfunction(cage_assets_generate_config)

function(cage_assets_add_data_path data)
	file(GLOB dirs "${data}/*")
	foreach(dir IN ITEMS ${dirs})
		if(IS_DIRECTORY ${dir})
			get_filename_component(name "${dir}" NAME)
			cage_directory_link("${dir}" "${cage_assets_intermediate_path}/data/${name}")
		endif()
	endforeach()
	#cage_assets_generate_config()
endfunction(cage_assets_add_data_path)

function(cage_assets_add_schemes_path schemes)
	file(GLOB fils "${schemes}/*.scheme")
	foreach(fil IN ITEMS ${fils})
		get_filename_component(name "${fil}" NAME)
		configure_file("${fil}" "${cage_assets_intermediate_path}/schemes/${name}" COPYONLY)
	endforeach()
	#cage_assets_generate_config()
endfunction(cage_assets_add_schemes_path)

function(cage_assets_init)
	if(NOT EXISTS "${cage_assets_intermediate_path}")
		file(MAKE_DIRECTORY "${cage_assets_intermediate_path}")
	endif()
	if(NOT EXISTS "${cage_assets_intermediate_path}/data")
		file(MAKE_DIRECTORY "${cage_assets_intermediate_path}/data")
	endif()
	if(NOT EXISTS "${cage_assets_intermediate_path}/schemes")
		file(MAKE_DIRECTORY "${cage_assets_intermediate_path}/schemes")
	endif()
	cage_assets_add_schemes_path("${cage_assets_current_list_dir}/../schemes")
	cage_assets_add_data_path("${cage_assets_current_list_dir}/../data")
	configure_file("${cage_assets_current_list_dir}/../sources/include/cage-engine/shaderConventions.h" "${cage_assets_intermediate_path}/data/cage/shader" COPYONLY)
	cage_assets_generate_config()
endfunction(cage_assets_init)

