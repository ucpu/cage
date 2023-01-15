#include <cage-core/systemInformation.h>
#include <cage-core/string.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include <Windows.h>
#include <intrin.h>
#include <powerbase.h> // CallNtPowerInformation
#include <Psapi.h> // GetProcessMemoryInfo
#pragma comment(lib, "PowrProf.lib") // CallNtPowerInformation
#pragma comment(lib, "Advapi32.lib") // RegGetValue
#else
#include <cage-core/concurrent.h>
#include <cage-core/process.h>
#include <cage-core/debug.h>
#endif

namespace cage
{
	String systemName()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		static_assert(sizeof(DWORD) == sizeof(uint32));
		const auto &readUint = [](StringPointer name) -> uint32
		{
			uint32 res = 0;
			DWORD sz = sizeof(res);
			auto ret = RegGetValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", name, RRF_RT_REG_DWORD, nullptr, &res, &sz);
			if (ret != ERROR_SUCCESS)
				return 0;
			return res;
		};
		const auto &readString = [](StringPointer name) -> String
		{
			String res;
			DWORD sz = String::MaxLength;
			auto ret = RegGetValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", name, RRF_RT_REG_SZ, nullptr, res.rawData(), &sz);
			if (ret != ERROR_SUCCESS)
				res.rawLength() = 0;
			else
				res.rawLength() = sz - 1;
			return res;
		};
		return Stringizer() + readString("ProductName") + " " + readString("DisplayVersion") + " (" + readString("CurrentVersion") + ", " + readUint("CurrentMajorVersionNumber") + "." + readUint("CurrentMinorVersionNumber") + ", " + readString("CurrentBuildNumber") + ", " + readString("EditionID") + ", " + readString("InstallationType") + ")";
#else
		Holder<Process> prg = newProcess(String("lsb_release -d"));
		String newName = prg->readLine();
		if (!isPattern(newName, "Description", "", ""))
		{
			// lsb_release is not installed, let's at least return full uname
			prg = newProcess(String("uname -a"));
			return prg->readLine();
		}

		newName = remove(newName, 0, String("Description:").length());
		String systemName = trim(newName);

		prg = newProcess(String("lsb_release -r"));
		newName = remove(prg->readLine(), 0, String("Release:").length());
		systemName = systemName + " " + trim(newName);

		prg = newProcess(String("uname -r"));
		systemName = systemName + ", kernel " + prg->readLine();

		return systemName;
#endif
	}

	String userName()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		char buf[String::MaxLength];
		buf[0] = 0;
		DWORD siz = String::MaxLength - 1;
		if (!GetUserName(buf, &siz))
			CAGE_THROW_ERROR(SystemError, "GetUserName", GetLastError());
		return buf;
#else
		Holder<Process> prg = newProcess(String("whoami"));
		return prg->readLine();
#endif
	}

	String hostName()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		char buf[String::MaxLength];
		buf[0] = 0;
		DWORD siz = String::MaxLength - 1;
		if (!GetComputerName(buf, &siz))
			CAGE_THROW_ERROR(SystemError, "GetComputerName", GetLastError());
		return buf;
#else
		Holder<Process> prg = newProcess(String("hostname"));
		return prg->readLine();
#endif
	}

	String processorName()
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
		Holder<Process> prg = newProcess(String("cat /proc/cpuinfo | grep -m 1 'model name' | cut -d: -f2-"));
		return trim(prg->readLine());
#endif
	}

	uint64 processorClockSpeed()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		// is there any better way than using CallNtPowerInformation?
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
		return ppi[0].MaxMhz * uint64(1000000);
#else
		try
		{
			detail::OverrideBreakpoint ob;
			Holder<Process> prg = newProcess(String("cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"));
			return toUint32(prg->readLine()) * 1000;
		}
		catch(const cage::Exception &)
		{
			// When the cpufreq driver is not loaded (file above does not exist), the CPU should be running on full speed
		}

		Holder<Process> prg = newProcess(String("cat /proc/cpuinfo | grep -m 1 'cpu MHz' | cut -d: -f2-"));
		return numeric_cast<uint64>(toDouble(trim(prg->readLine())) * 1e6);
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
#else
		Holder<Process> prg = newProcess(String("cat /proc/meminfo | grep -m 1 'MemTotal' | awk '{print $2}'"));
		return toUint64(prg->readLine()) * 1024;
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
#else
		try
		{
			detail::OverrideBreakpoint ob;
			Holder<Process> prg = newProcess(String("cat /proc/meminfo | grep -m 1 'MemAvailable' | awk '{print $2}'"));
			return toUint64(prg->readLine()) * 1024;
		}
		catch (const cage::Exception &)
		{
			// On some systems the "MemAvailable" is not present, in which case continue to look for "MemFree"
		}

		Holder<Process> prg = newProcess(String("cat /proc/meminfo | grep -m 1 'MemFree' | awk '{print $2}'"));
		return toUint64(prg->readLine()) * 1024;
#endif
	}

	uint64 memoryUsed()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		PROCESS_MEMORY_COUNTERS m;
		if (GetProcessMemoryInfo(GetCurrentProcess(), &m, sizeof(m)) == 0)
			CAGE_THROW_ERROR(SystemError, "GetProcessMemoryInfo", GetLastError());
		return m.WorkingSetSize;
#else
		Holder<Process> prg = newProcess({ Stringizer() + "cat /proc/" + currentProcessId() + "/status | grep -m 1 'VmRSS' | awk '{print $2}'" });
		return toUint64(prg->readLine()) * 1024;
#endif
	}
}

