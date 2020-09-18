cmake_minimum_required(VERSION 3.10)

message(STATUS "----------------------------------------------------------------")
message(STATUS "libzip")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")
option(LIBZIP_DO_INSTALL "Install libzip and the related files" OFF)
option(ENABLE_COMMONCRYPTO "Enable use of CommonCrypto" OFF)
option(ENABLE_GNUTLS "Enable use of GnuTLS" OFF)
option(ENABLE_MBEDTLS "Enable use of mbed TLS" OFF)
option(ENABLE_OPENSSL "Enable use of OpenSSL" OFF)
option(ENABLE_WINDOWS_CRYPTO "Enable use of Windows cryptography libraries" OFF)
option(ENABLE_BZIP2 "Enable use of BZip2" OFF)
option(ENABLE_LZMA "Enable use of LZMA" OFF)
option(BUILD_TOOLS "Build tools in the src directory (zipcmp, zipmerge, ziptool)" OFF)
option(BUILD_REGRESS "Build regression tests" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_DOC "Build documentation" OFF)
set(ZLIB_VERSION_STRING "1.2.11")

add_subdirectory(libzip)

# get rid of a warning that the property is not honored
set_property(TARGET zip PROPERTY C_VISIBILITY_PRESET)

target_include_directories(zip INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libzip/lib")
target_include_directories(zip INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/libzip")
target_compile_definitions(zip INTERFACE ZIP_DISABLE_DEPRECATED)



