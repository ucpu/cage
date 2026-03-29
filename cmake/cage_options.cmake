
option(cage_faster_debug "enable some optimizations to improve performance in debug builds" ON)
option(cage_faster_release "enable more aggressive inlining in release builds" OFF)
option(cage_assert_in_release_enabled "enable runtime asserts in non-debug builds" OFF)
option(cage_validate_graphics "enable runtime validation in webgpu/dawn" ON)
option(cage_profiling_enabled "enable compiling-in instructions for profiling" ON)
option(cage_use_steam_sockets "include Game Networking Sockets library by Valve/Steam" OFF)
