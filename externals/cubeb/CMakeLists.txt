cmake_minimum_required(VERSION 3.16)

message(STATUS "----------------------------------------------------------------")
message(STATUS "cubeb")
message(STATUS "----------------------------------------------------------------")

# base library

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

add_library(cubeb STATIC
	${CMAKE_CURRENT_BINARY_DIR}/cubeb/exports/cubeb_export.h
	cubeb/include/cubeb/cubeb.h
	cubeb/src/cubeb.c
	cubeb/src/cubeb_mixer.cpp
	cubeb/src/cubeb_resampler.cpp
	cubeb/src/cubeb_log.cpp
	cubeb/src/cubeb_strings.c
	cubeb/src/cubeb_utils.cpp
	cubeb/src/speex/resample.c
)
target_include_directories(cubeb PUBLIC cubeb/include ${CMAKE_CURRENT_BINARY_DIR}/cubeb/exports PRIVATE cubeb/src)
target_compile_definitions(cubeb PRIVATE OUTSIDE_SPEEX FLOATING_POINT EXPORT= RANDOM_PREFIX=speex)
if(UNIX)
	target_link_libraries(cubeb PRIVATE pthread ${CMAKE_DL_LIBS})
endif()

include(GenerateExportHeader)
generate_export_header(cubeb EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/cubeb/exports/cubeb_export.h)

# backends

include(CheckIncludeFiles)

check_include_files(pulse/pulseaudio.h USE_PULSE)
if(USE_PULSE)
	target_sources(cubeb PRIVATE cubeb/src/cubeb_pulse.c)
	target_compile_definitions(cubeb PRIVATE USE_PULSE)
endif()

check_include_files(alsa/asoundlib.h USE_ALSA)
if(USE_ALSA)
	target_sources(cubeb PRIVATE cubeb/src/cubeb_alsa.c)
	target_compile_definitions(cubeb PRIVATE USE_ALSA)
endif()

check_include_files(jack/jack.h USE_JACK)
if(USE_JACK)
	target_sources(cubeb PRIVATE cubeb/src/cubeb_jack.cpp)
	target_compile_definitions(cubeb PRIVATE USE_JACK)
endif()

check_include_files(audioclient.h USE_WASAPI)
if(USE_WASAPI)
	target_sources(cubeb PRIVATE cubeb/src/cubeb_wasapi.cpp)
	target_compile_definitions(cubeb PRIVATE USE_WASAPI)
	target_link_libraries(cubeb PRIVATE avrt ole32)
endif()

check_include_files("windows.h;mmsystem.h" USE_WINMM)
if(USE_WINMM)
	target_sources(cubeb PRIVATE cubeb/src/cubeb_winmm.c)
	target_compile_definitions(cubeb PRIVATE USE_WINMM)
	target_link_libraries(cubeb PRIVATE winmm)
endif()

check_include_files(sndio.h USE_SNDIO)
if(USE_SNDIO)
	target_sources(cubeb PRIVATE cubeb/src/cubeb_sndio.c)
	target_compile_definitions(cubeb PRIVATE USE_SNDIO)
endif()

