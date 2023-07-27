
set(cage_copy_redist_current_list_dir "${CMAKE_CURRENT_LIST_DIR}")
set(cage_copy_redist_path "${CMAKE_BINARY_DIR}/result/redist" CACHE PATH "Directory to save all redistributable files (eg. licenses) to.")

function(cage_copy_redist_license sourcePath targetName)
	configure_file("${sourcePath}" "${cage_copy_redist_path}/licenses/${targetName}" COPYONLY)
endfunction(cage_copy_redist_license)

function(cage_copy_redist)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/assimp/assimp/LICENSE" "assimp" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/avir/avir/LICENSE" "avir" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/bc7enc_rdo/bc7enc_rdo/LICENSE" "bc7enc_rdo" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/cubeb/cubeb/LICENSE" "cubeb" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/dr_libs/dr_libs/README.md" "dr_libs" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/dualmc/dualmc/LICENSE" "dualmc" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/fastnoise/fastnoise/LICENSE" "fastnoise" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/freetype/freetype/LICENSE.TXT" "freetype" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/freetype/freetype/docs/FTL.TXT" "freetype_ftl" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/freetype/freetype/src/bdf/README" "freetype_bdf" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/freetype/freetype/src/pcf/README" "freetype_pcf" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/freetype/freetype/src/gzip/zlib.h" "freetype_gzip" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/glfw/glfw/LICENSE.md" "glfw" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/glm/glm/manual.md" "glm" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/hsluv/hsluv.h" "hsluv" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/jpeg/jpeg/LICENSE.md" "jpeg" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/msdfgen/msdfgen/LICENSE.txt" "msdfgen" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/ogg/ogg/COPYING" "ogg" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/openxr-sdk/OpenXR-SDK/LICENSE" "openxr-sdk" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/plf/plf_colony/LICENSE.md" "plf" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/pmp/pmp/external/eigen-3.4.0/COPYING.MPL2" "pmp_eigen" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/pmp/pmp/LICENSE.txt" "pmp" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/png/png/LICENSE" "png" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/samplerate/libsamplerate/COPYING" "samplerate" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/simplefilewatcher/simplefilewatcher/License.txt" "simplefilewatcher" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/stb/stb/LICENSE" "stb" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/tiff/tiff/LICENSE.md" "tiff" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/tinyexr/tinyexr/README.md" "tinyexr" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/unordered_dense/unordered_dense/LICENSE" "unordered_dense" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/utfcpp/utfcpp/LICENSE" "utfcpp" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/vorbis/vorbis/COPYING" "vorbis" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/xatlas/xatlas/LICENSE" "xatlas" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/zlib-ng/zlib-ng/LICENSE.md" "zlib-ng" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../externals/zstd/zstd/LICENSE" "zstd" COPYONLY)
	cage_copy_redist_license("${cage_copy_redist_current_list_dir}/../LICENSE" "cage" COPYONLY)
endfunction(cage_copy_redist)

