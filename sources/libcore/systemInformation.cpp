#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>
#include <cage-core/systemInformation.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include <Windows.h>
#include <intrin.h>
#include <powerbase.h> // is there any better way than using CallNtPowerInformation?
#pragma comment(lib, "PowrProf.lib")
#else
// todo
#endif

namespace cage
{
	string systemName()
	{
		// todo
		return "";
	}

	string userName()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		char buf[string::MaxLength];
		buf[0] = 0;
		DWORD siz = string::MaxLength - 1;
		if (!GetUserName(buf, &siz))
			CAGE_THROW_ERROR(systemError, "GetUserName", GetLastError());
		return buf;
#else
		// todo
		return "";
#endif
	}

	string hostName()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		char buf[string::MaxLength];
		buf[0] = 0;
		DWORD siz = string::MaxLength - 1;
		if (!GetComputerName(buf, &siz))
			CAGE_THROW_ERROR(systemError, "GetComputerName", GetLastError());
		return buf;
#else
		// todo
		return "";
#endif
	}

	string processorName()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		// https://vcpptips.wordpress.com/2012/12/30/how-to-get-the-cpu-name/
		sint32 CPUInfo[4] = { -1 };
		__cpuid(CPUInfo, 0x80000000);
		uint32 nExIds = CPUInfo[0];
		char CPUBrandString[0x40];
		memset(CPUBrandString, 0, sizeof(CPUBrandString));
		for (int i = 0; i < 3; i++)
		{
			if (0x80000002 + i > nExIds)
				break;
			__cpuid(CPUInfo, 0x80000002 + i);
			memcpy(CPUBrandString + (16 * i), CPUInfo, sizeof(CPUInfo));
		}
		return CPUBrandString;
#else
		// todo
		return "";
#endif
	}

	uint64 processorClockSpeed()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		struct PPI
		{
		ULONG Number;
		ULONG MaxMhz;
		ULONG CurrentMhz;
		ULONG MhzLimit;
		ULONG MaxIdleState;
		ULONG CurrentIdleState;
		} ppi[32];
		memset(&ppi, 0, sizeof(ppi));
		auto ret = CallNtPowerInformation(ProcessorInformation, nullptr, 0, &ppi, sizeof(ppi));
		if (ret != 0)
			CAGE_THROW_ERROR(systemError, "CallNtPowerInformation", ret);
		return ppi[0].MaxMhz * 1000000;
#else
		// todo
		return 0;
#endif
	}

	uint64 memoryCapacity()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		MEMORYSTATUSEX m;
		m.dwLength = sizeof(m);
		if (!GlobalMemoryStatusEx(&m))
			CAGE_THROW_ERROR(systemError, "GlobalMemoryStatusEx", GetLastError());
		return m.ullTotalPhys;
#else
		// todo
		return 0;
#endif
	}

	uint64 memoryAvailable()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		MEMORYSTATUSEX m;
		m.dwLength = sizeof(m);
		if (!GlobalMemoryStatusEx(&m))
			CAGE_THROW_ERROR(systemError, "GlobalMemoryStatusEx", GetLastError());
		return m.ullAvailPhys;
#else
		// todo
		return 0;
#endif
	}
}

