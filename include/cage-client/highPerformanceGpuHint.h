
#include <cage-core/core.h>

#ifdef CAGE_SYSTEM_WINDOWS

#include <windows.h>
extern "C"
{
	GCHL_API_EXPORT DWORD NvOptimusEnablement = 1;
	GCHL_API_EXPORT int AmdPowerXpressRequestHighPerformance = 1;
}

#else

extern "C"
{
	GCHL_API_EXPORT int NvOptimusEnablement = 1;
	GCHL_API_EXPORT int AmdPowerXpressRequestHighPerformance = 1;
}

#endif // CAGE_SYSTEM_WINDOWS
