
set(cage_copy_redist_current_list_dir "${CMAKE_CURRENT_LIST_DIR}")
set(cage_copy_redist_path "${CMAKE_BINARY_DIR}/result/redist" CACHE PATH "Directory to save all redistributable files (eg. licenses) to.")

function(cage_copy_redist_license sourcePath targetName)
	configure_file("${sourcePath}" "${cage_copy_redist_path}/licenses/${targetName}" COPYONLY)
endfunction(cage_copy_redist_license)

function(cage_copy_redist)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/assimp/assimp/LICENSE" "assimp" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/dr_libs/dr_libs/README.md" "drlibs" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/fastnoise/fastnoise/LICENSE" "fastnoise" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/freetype/freetype/docs/LICENSE.TXT" "freetype" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/freetype/freetype/docs/FTL.TXT" "freetype_ftl" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/glfw/glfw/LICENSE.md" "glfw" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/glm/glm/manual.md" "glm" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/jpeg/jpeg/LICENSE.md" "jpeg" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/libpng/libpng/LICENSE" "png" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/cubeb/cubeb/LICENSE" "cubeb" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/libtiff/libtiff/COPYRIGHT" "tiff" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/libzip/libzip/LICENSE" "zip" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/msdfgen/msdfgen/LICENSE.txt" "msdfgen" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/ogg/ogg/COPYING" "ogg" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/optick/optick/LICENSE" "optick" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/simplefilewatcher/simplefilewatcher/License.txt" "simplefilewatcher" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/utfcpp/utfcpp/LICENSE" "utfcpp" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/vorbis/vorbis/COPYING" "vorbis" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/xsimd/xsimd/LICENSE" "xsimd" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/zlib-ng/zlib-ng/LICENSE.md" "zlib-ng" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/xatlas/xatlas/LICENSE" "xatlas" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/pmp/pmp/external/eigen/COPYING.MPL2" "pmp_eigen" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/pmp/pmp/external/rply/LICENSE" "pmp_rply" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/pmp/pmp/LICENSE.txt" "pmp" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/dualmc/dualmc/LICENSE" "dualmc" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/plf/plf_colony/LICENSE.md" "plf" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/zstd/zstd/LICENSE" "zstd" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../LICENSE" "cage" COPYONLY)
endfunction(cage_copy_redist)

