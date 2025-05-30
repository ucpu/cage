message(STATUS "----------------------------------------------------------------")
message(STATUS "cubeb")
message(STATUS "----------------------------------------------------------------")

# base library

add_library(cubeb STATIC
	cubeb/src/cubeb.c
	cubeb/src/cubeb_mixer.cpp
	cubeb/src/cubeb_resampler.cpp
	cubeb/src/cubeb_log.cpp
	cubeb/src/cubeb_strings.c
	cubeb/src/cubeb_utils.cpp
	cubeb/subprojects/speex/resample.c
	${CMAKE_CURRENT_BINARY_DIR}/cubeb/exports/cubeb_export.h
)
target_include_directories(cubeb PUBLIC cubeb/include ${CMAKE_CURRENT_BINARY_DIR}/cubeb/exports PRIVATE cubeb/src cubeb/subprojects)
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

check_include_files(sndio.h USE_SNDIO)
if(USE_SNDIO)
	target_sources(cubeb PRIVATE cubeb/src/cubeb_sndio.c)
	target_compile_definitions(cubeb PRIVATE USE_SNDIO)
endif()

check_include_files(aaudio/AAudio.h USE_AAUDIO)
if(USE_AAUDIO)
	target_sources(cubeb PRIVATE cubeb/src/cubeb_aaudio.cpp)
	target_compile_definitions(cubeb PRIVATE USE_AAUDIO)
	target_compile_definitions(cubeb PRIVATE CUBEB_AAUDIO_LOW_LATENCY)
endif()

check_include_files(AudioUnit/AudioUnit.h USE_AUDIOUNIT)
if(USE_AUDIOUNIT)
	target_sources(cubeb PRIVATE cubeb/src/cubeb_audiounit.cpp cubeb/src/cubeb_osx_run_loop.cpp)
	target_compile_definitions(cubeb PRIVATE USE_AUDIOUNIT)
	target_link_libraries(cubeb PRIVATE "-framework AudioUnit" "-framework CoreAudio" "-framework CoreServices")
endif()

check_include_files(audioclient.h USE_WASAPI)
if(USE_WASAPI)
	target_sources(cubeb PRIVATE cubeb/src/cubeb_wasapi.cpp)
	target_compile_definitions(cubeb PRIVATE USE_WASAPI)
	target_link_libraries(cubeb PRIVATE avrt ole32 ksuser winmm)
endif()

if(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION VERSION_GREATER_EQUAL "10.0.18362.0")
	check_include_files("windows.h;mmsystem.h" USE_WINMM)
	if(USE_WINMM)
		target_sources(cubeb PRIVATE cubeb/src/cubeb_winmm.c)
		target_compile_definitions(cubeb PRIVATE USE_WINMM)
		target_link_libraries(cubeb PRIVATE ksuser winmm)
	endif()
else()
	message(STATUS "winmm backend is skipped due to bug in old version of windows sdk")
endif()

check_include_files(SLES/OpenSLES.h USE_OPENSL)
if(USE_OPENSL)
	target_sources(cubeb PRIVATE cubeb/src/cubeb_opensl.c cubeb/src/cubeb-jni.cpp)
	target_compile_definitions(cubeb PRIVATE USE_OPENSL)
	target_link_libraries(cubeb PRIVATE OpenSLES)
endif()

check_include_files(android/log.h USE_AUDIOTRACK)
if(USE_AUDIOTRACK)
	target_sources(cubeb PRIVATE cubeb/src/cubeb_audiotrack.c)
	target_compile_definitions(cubeb PRIVATE USE_AUDIOTRACK)
	target_link_libraries(cubeb PRIVATE log)
endif()

check_include_files(sys/audioio.h USE_SUN)
if(USE_SUN)
	target_sources(cubeb PRIVATE cubeb/src/cubeb_sun.c)
	target_compile_definitions(cubeb PRIVATE USE_SUN)
endif()

check_include_files(kai.h USE_KAI)
if(USE_KAI)
	target_sources(cubeb PRIVATE cubeb/src/cubeb_kai.c)
	target_compile_definitions(cubeb PRIVATE USE_KAI)
	target_link_libraries(cubeb PRIVATE kai)
endif()


