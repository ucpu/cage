#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>
#include <cage-core/systemInformation.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include <Windows.h>
#include <intrin.h>
#include <powerbase.h> // is there any better way than using CallNtPowerInformation?
#pragma comment(lib, "PowrProf.lib")
#elif defined(CAGE_SYSTEM_LINUX)
#include <cage-core/process.h>
#else
// todo
#endif

namespace cage
{
	string systemName()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		// todo
		return "";
#elif defined(CAGE_SYSTEM_LINUX)
		Holder<Process> prg = newProcess(string("lsb_release -d"));
		string newName = prg->readLine();
		if (!newName.isPattern("Description", "", ""))
		{
			// lsb_release is not installed, let's at least return full uname
			prg = newProcess(string("uname -a"));
			return prg->readLine();
		}

		newName = newName.remove(0, string("Description:").length());
		string systemName = newName.trim();

		prg = newProcess(string("lsb_release -r"));
		newName = prg->readLine().remove(0, string("Release:").length());
		systemName = systemName + " " + newName.trim();

		prg = newProcess(string("uname -r"));
		systemName = systemName + ", kernel " + prg->readLine();

		return systemName;
#else
		// todo
		return "";
#endif
	}

	string userName()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		char buf[string::MaxLength];
		buf[0] = 0;
		DWORD siz = string::MaxLength - 1;
		if (!GetUserName(buf, &siz))
			CAGE_THROW_ERROR(SystemError, "GetUserName", GetLastError());
		return buf;
#elif defined(CAGE_SYSTEM_LINUX)
		Holder<Process> prg = newProcess(string("whoami"));
		return prg->readLine();
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
			CAGE_THROW_ERROR(SystemError, "GetComputerName", GetLastError());
		return buf;
#elif defined(CAGE_SYSTEM_LINUX)
		Holder<Process> prg = newProcess(string("hostname"));
		return prg->readLine();
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
#elif defined(CAGE_SYSTEM_LINUX)
		Holder<Process> prg = newProcess(string("cat /proc/cpuinfo | grep -m 1 'model name' | cut -d: -f2-"));
		return prg->readLine().trim();
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
			CAGE_THROW_ERROR(SystemError, "CallNtPowerInformation", ret);
		return ppi[0].MaxMhz * 1000000;
#elif defined(CAGE_SYSTEM_LINUX)
		try
		{
			Holder<Process> prg = newProcess(string("cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"));
			return prg->readLine().toUint32() * 1000;
		}
		catch(const cage::Exception &)
		{
			// When the cpufreq driver is not loaded (file above does not exist), the CPU should be running on full speed
		}

		Holder<Process> prg = newProcess(string("cat /proc/cpuinfo | grep -m 1 'cpu MHz' | cut -d: -f2-"));
		return numeric_cast<uint64>(prg->readLine().trim().toDouble() * 1e6);
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
			CAGE_THROW_ERROR(SystemError, "GlobalMemoryStatusEx", GetLastError());
		return m.ullTotalPhys;
#elif defined(CAGE_SYSTEM_LINUX)
		Holder<Process> prg = newProcess(string("cat /proc/meminfo | grep -m 1 'MemTotal' | awk '{print $2}'"));
		return prg->readLine().toUint64() * 1024;
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
			CAGE_THROW_ERROR(SystemError, "GlobalMemoryStatusEx", GetLastError());
		return m.ullAvailPhys;
#elif defined(CAGE_SYSTEM_LINUX)
		try
		{
			Holder<Process> prg = newProcess(string("cat /proc/meminfo | grep -m 1 'MemAvailable' | awk '{print $2}'"));
			return prg->readLine().toUint64() * 1024;
		}
		catch (const cage::Exception &)
		{
			// On some systems the "MemAvailable" is not present, in which case continue to look for "MemFree"
		}

		Holder<Process> prg = newProcess(string("cat /proc/meminfo | grep -m 1 'MemFree' | awk '{print $2}'"));
		return prg->readLine().toUint64() * 1024;
#else
		// todo
		return 0;
#endif
	}
}

