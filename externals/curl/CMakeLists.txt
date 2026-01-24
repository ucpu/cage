message(STATUS "----------------------------------------------------------------")
message(STATUS "curl")
message(STATUS "----------------------------------------------------------------")

if (WIN32)
	set(CURL_USE_SCHANNEL ON CACHE INTERNAL "")
else()
	if (cage_use_steam_sockets)
		# steam sockets use openssl already, so use it here too
		find_package(OpenSSL REQUIRED)
		set(CURL_USE_OpenSSL ON CACHE INTERNAL "")
		set(HAVE_DES_ECB_ENCRYPT OFF CACHE INTERNAL "") # bypass a check that would otherwise fail
		set(HAVE_SSL_SET0_WBIO OFF CACHE INTERNAL "") # bypass a check that would otherwise fail
		set(CURL_DISABLE_SRP ON CACHE INTERNAL "") # bypass a check that would otherwise fail
	else()
		find_package(MBEDTLS REQUIRED)
		set(CURL_USE_MBEDTLS ON CACHE INTERNAL "")
		set(HAVE_MBEDTLS_DES_CRYPT_ECB OFF CACHE INTERNAL "") # bypass a check that would otherwise fail
	endif()
endif()

find_package(ZSTD REQUIRED)
find_package(ZLIB REQUIRED)

set(BUILD_CURL_EXE OFF CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(BUILD_STATIC_LIBS ON CACHE INTERNAL "")
set(CURL_DISABLE_INSTALL ON CACHE INTERNAL "")
set(CURL_USE_PKGCONFIG OFF CACHE INTERNAL "")
set(HTTP_ONLY ON CACHE INTERNAL "")
set(BUILD_LIBCURL_DOCS OFF CACHE INTERNAL "")
set(BUILD_MISC_DOCS OFF CACHE INTERNAL "")
set(ENABLE_CURL_MANUAL OFF CACHE INTERNAL "")
set(CURL_USE_LIBPSL OFF CACHE INTERNAL "")
set(BUILD_TESTING OFF CACHE INTERNAL "")
set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(CURL_ENABLE_EXPORT_TARGET OFF CACHE INTERNAL "")

add_subdirectory(curl)

cage_gen_module(curl $<TARGET_LINKER_FILE:libcurl> "${CMAKE_CURRENT_SOURCE_DIR}/curl/include")

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/curl/include/curl/curlver.h" curl_version REGEX "#define LIBCURL_VERSION ")
string(REGEX REPLACE "#define LIBCURL_VERSION \"([^\"]+)\"" "\\1" curl_version "${curl_version}")
message(STATUS "curl version: ${curl_version}")
set(filename "${CAGE_EXTERNALS_MODULE_PATH}/FindCURL.cmake")
file(APPEND ${filename} "set(CURL_VERSION_STRING ${curl_version})\n")
