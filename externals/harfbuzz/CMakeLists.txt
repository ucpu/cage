cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "harfbuzz")
message(STATUS "----------------------------------------------------------------")

# enabling this would cause some tests to fail
#   because the test compilations will not find the freetype library
#   which is not yet built (during cmake configure phase)
#option(HB_HAVE_FREETYPE "Enable freetype interop helpers" ON)

option(HB_BUILD_SUBSET "Build harfbuzz-subset" OFF)
option(HB_BUILD_TESTS "Build harfbuzz tests" OFF)
option(HB_HAVE_CORETEXT "Enable CoreText shaper backend on macOS" OFF)
set(SKIP_INSTALL_ALL ON)

add_subdirectory(harfbuzz)

target_include_directories(harfbuzz PUBLIC harfbuzz/src)


# restore freetype interop helpers
#find_package(Freetype REQUIRED)
#target_sources(harfbuzz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/harfbuzz/src/hb-ft.cc ${CMAKE_CURRENT_SOURCE_DIR}/harfbuzz/src/hb-ft.h)
#target_compile_definitions(harfbuzz PUBLIC HAVE_FREETYPE=1)
#target_include_directories(harfbuzz PUBLIC ${FREETYPE_INCLUDE_DIRS})

