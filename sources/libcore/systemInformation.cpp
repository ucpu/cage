#include <cage-core/concurrent.h>
#include <cage-core/systemInformation.h>
#include <cage-core/string.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include <Windows.h>
#include <intrin.h>
#include <powerbase.h> // CallNtPowerInformation
#pragma comment(lib, "PowrProf.lib") // CallNtPowerInformation
#pragma comment(lib, "Advapi32.lib") // RegGetValue
#else
#include <cage-core/process.h>
#endif

namespace cage
{
	string systemName()
	{
#ifdef CAGE_SYSTEM_WINDOWS
		static_assert(sizeof(DWORD) == sizeof(uint32));
		const auto &readUint = [](StringLiteral name) -> uint32
		{
			uint32 res = 0;
			DWORD sz = sizeof(res);
			auto ret = RegGetValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", name, RRF_RT_REG_DWORD, nullptr, &res, &sz);
			if (ret != ERROR_SUCCESS)
				return 0;
			return res;
		};
		const auto &readString = [](StringLiteral name) -> string
		{
			string res;
			DWORD sz = string::MaxLength;
			auto ret = RegGetValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", name, RRF_RT_REG_SZ, nullptr, res.rawData(), &sz);
			if (ret != ERROR_SUCCESS)
				res.rawLength() = 0;
			else
				res.rawLength() = sz - 1;
			return res;
		};
		return stringizer() + readString("ProductName") + " " + readString("DisplayVersion") + " (" + readString("CurrentVersion") + ", " + readUint("CurrentMajorVersionNumber") + "." + readUint("CurrentMinorVersionNumber") + ", " + readString("CurrentBuildNumber") + ", " + readString("EditionID") + ", " + readString("InstallationType") + ")";
#else
		Holder<Process> prg = newProcess(string("lsb_release -d"));
		string newName = prg->readLine();
		if (!isPattern(newName, "Description", "", ""))
		{
			// lsb_release is not installed, let's at least return full uname
			prg = newProcess(string("uname -a"));
			return prg->readLine();
		}

		newName = remove(newName, 0, string("Description:").length());
		string systemName = trim(newName);

		prg = newProcess(string("lsb_release -r"));
		newName = remove(prg->readLine(), 0, string("Release:").length());
		systemName = systemName + " " + trim(newName);

		prg = newProcess(string("uname -r"));
		systemName = systemName + ", kernel " + prg->readLine();

		return systemName;
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
#else
		Holder<Process> prg = newProcess(string("whoami"));
		return prg->readLine();
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
#else
		Holder<Process> prg = newProcess(string("hostname"));
		return prg->readLine();
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
		Holder<Process> prg = newProcess(string("cat /proc/cpuinfo | grep -m 1 'model name' | cut -d: -f2-"));
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
			Holder<Process> prg = newProcess(string("cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"));
			return toUint32(prg->readLine()) * 1000;
		}
		catch(const cage::Exception &)
		{
			// When the cpufreq driver is not loaded (file above does not exist), the CPU should be running on full speed
		}

		Holder<Process> prg = newProcess(string("cat /proc/cpuinfo | grep -m 1 'cpu MHz' | cut -d: -f2-"));
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
		Holder<Process> prg = newProcess(string("cat /proc/meminfo | grep -m 1 'MemTotal' | awk '{print $2}'"));
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
			Holder<Process> prg = newProcess(string("cat /proc/meminfo | grep -m 1 'MemAvailable' | awk '{print $2}'"));
			return toUint64(prg->readLine()) * 1024;
		}
		catch (const cage::Exception &)
		{
			// On some systems the "MemAvailable" is not present, in which case continue to look for "MemFree"
		}

		Holder<Process> prg = newProcess(string("cat /proc/meminfo | grep -m 1 'MemFree' | awk '{print $2}'"));
		return toUint64(prg->readLine()) * 1024;
#endif
	}
}

