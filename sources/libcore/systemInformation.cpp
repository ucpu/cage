#ifdef CAGE_SYSTEM_WINDOWS
	#include "windowsMinimumInclude.h" // must be included first
	#include <Psapi.h> // GetProcessMemoryInfo
	#include <intrin.h>
	#include <powerbase.h> // CallNtPowerInformation
	#pragma comment(lib, "PowrProf.lib") // CallNtPowerInformation
	#pragma comment(lib, "Advapi32.lib") // RegGetValue
#else
	#include <algorithm>
	#include <cstdlib> // getenv
	#include <sstream>
	#include <string>
	#ifdef CAGE_SYSTEM_MAC
		#include <mach/mach.h>
		#include <mach/vm_statistics.h>
		#include <mach/mach_host.h>
		#include <sys/sysctl.h>
		#include <unistd.h> // gethostname
	#endif
#endif

#include <cage-core/concurrent.h> // currentProcessId
#include <cage-core/debug.h>
#include <cage-core/files.h>
#include <cage-core/string.h>
#include <cage-core/systemInformation.h>

namespace cage
{
#ifndef CAGE_SYSTEM_WINDOWS
	namespace
	{
		std::string getFile(const String &path)
		{
			Holder<File> f = readFile(path);
			std::string res;
			res.reserve(1000);
			try
			{
				detail::OverrideException o;
				char p = 0;
				while (true)
				{
					char c = 0;
					f->read(PointerRange<char>(&c, &c + 1));
					res += c;
					if (c == '\n' && p == '\n')
						break; // empty line
					p = c;
				}
			}
			catch (...)
			{
				// done
			}
			return res;
		}

		std::string grepLine(const std::string &data, const char *pattern)
		{
			std::istringstream stream(data);
			std::string line;
			while (std::getline(stream, line))
			{
				auto p = line.find(pattern);
				if (p != std::string::npos)
					return line;
			}
			return "";
		}
	}
#endif

	String systemName()
	{
		try
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
#elif defined(CAGE_SYSTEM_MAC)
			// Get macOS version using sysctlbyname
			char versionStr[256] = {0};
			size_t size = sizeof(versionStr);
			sysctlbyname("kern.osproductversion", versionStr, &size, nullptr, 0);

			// Get build version
			char buildStr[256] = {0};
			size = sizeof(buildStr);
			sysctlbyname("kern.osversion", buildStr, &size, nullptr, 0);

			// Get kernel version
			char kernelStr[256] = {0};
			size = sizeof(kernelStr);
			sysctlbyname("kern.version", kernelStr, &size, nullptr, 0);

			// Extract kernel version number from the full string
			String kernel = kernelStr;
			uint32 end = find(kernel, ":");
			if (end != m)
				kernel = subString(kernel, 0, end);

			return Stringizer() + "macOS " + versionStr + " (" + buildStr + "), kernel: " + kernel;
#else
			std::string name = getFile("/etc/os-release");
			name = grepLine(name, "PRETTY_NAME");
			String n = name.c_str();
			split(n, "=");
			n = replace(n, "\"", "");
			std::string kernel = getFile("/proc/sys/kernel/osrelease");
			return Stringizer() + n + ", kernel: " + trim(String(kernel.c_str()));
#endif
		}
		catch (...)
		{
			return "";
		}
	}

	String userName()
	{
		try
		{
#ifdef CAGE_SYSTEM_WINDOWS
			char buf[String::MaxLength];
			buf[0] = 0;
			DWORD siz = String::MaxLength - 1;
			if (!GetUserName(buf, &siz))
				CAGE_THROW_ERROR(SystemError, "GetUserName", GetLastError());
			return buf;
#else
			const char *name = getenv("USER");
			if (!name)
				name = getenv("LOGNAME");
			return name;
#endif
		}
		catch (...)
		{
			return "";
		}
	}

	String hostName()
	{
		try
		{
#ifdef CAGE_SYSTEM_WINDOWS
			char buf[String::MaxLength];
			buf[0] = 0;
			DWORD siz = String::MaxLength - 1;
			if (!GetComputerName(buf, &siz))
				CAGE_THROW_ERROR(SystemError, "GetComputerName", GetLastError());
			return buf;
#elif defined(CAGE_SYSTEM_MAC)
			// Use gethostname on macOS
			char buf[String::MaxLength];
			if (gethostname(buf, sizeof(buf)) != 0)
				return "";
			return buf;
#else
			return trim(String(getFile("/proc/sys/kernel/hostname").c_str()));
#endif
		}
		catch (...)
		{
			return "";
		}
	}

	String processorName()
	{
		try
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
#elif defined(CAGE_SYSTEM_MAC)
			// Use sysctlbyname to get CPU brand string on macOS
			char buf[String::MaxLength];
			size_t size = sizeof(buf);
			if (sysctlbyname("machdep.cpu.brand_string", buf, &size, nullptr, 0) != 0)
				return "";
			return buf;
#else
			std::string cpu = getFile("/proc/cpuinfo");
			cpu = grepLine(cpu, "model name");
			String c = cpu.c_str();
			split(c, ":");
			c = trim(c);
			return c;
#endif
		}
		catch (...)
		{
			return "";
		}
	}

	uint64 processorClockSpeed()
	{
		try
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
#elif defined(CAGE_SYSTEM_MAC)
			// CPU frequency not available on Apple Silicon
			return 0;
#else
			std::string cpu = getFile("/proc/cpuinfo");
			cpu = grepLine(cpu, "cpu MHz");
			String c = cpu.c_str();
			split(c, ":");
			c = trim(c);
			return toDouble(c) * 1'000'000;
#endif
		}
		catch (...)
		{
			return 0;
		}
	}

	uint64 memoryCapacity()
	{
		try
		{
#ifdef CAGE_SYSTEM_WINDOWS
			MEMORYSTATUSEX m;
			m.dwLength = sizeof(m);
			if (!GlobalMemoryStatusEx(&m))
				CAGE_THROW_ERROR(SystemError, "GlobalMemoryStatusEx", GetLastError());
			return m.ullTotalPhys;
#elif defined(CAGE_SYSTEM_MAC)
			// Use sysctlbyname to get total memory on macOS
			uint64 mem = 0;
			size_t size = sizeof(mem);
			if (sysctlbyname("hw.memsize", &mem, &size, nullptr, 0) != 0)
				return 0;
			return mem;
#else
			std::string mem = getFile("/proc/meminfo");
			mem = grepLine(mem, "MemTotal");
			String c = mem.c_str();
			split(c, ":");
			c = trim(c);
			c = split(c); // split kB
			c = trim(c);
			return toUint64(c) * 1024;
#endif
		}
		catch (...)
		{
			return 0;
		}
	}

	uint64 memoryAvailable()
	{
		try
		{
#ifdef CAGE_SYSTEM_WINDOWS
			MEMORYSTATUSEX m;
			m.dwLength = sizeof(m);
			if (!GlobalMemoryStatusEx(&m))
				CAGE_THROW_ERROR(SystemError, "GlobalMemoryStatusEx", GetLastError());
			return m.ullAvailPhys;
#elif defined(CAGE_SYSTEM_MAC)
			// Get available memory on macOS using vm_stat
			vm_size_t page_size;
			host_page_size(mach_host_self(), &page_size);
			vm_statistics64_data_t vm_stat;
			mach_msg_type_number_t host_size = sizeof(vm_stat) / sizeof(natural_t);
			if (host_statistics64(mach_host_self(), HOST_VM_INFO, (host_info64_t)&vm_stat, &host_size) != KERN_SUCCESS)
				return 0;
			return (uint64)(vm_stat.free_count + vm_stat.inactive_count) * page_size;
#else
			std::string mem = getFile("/proc/meminfo");
			mem = grepLine(mem, "MemAvailable");
			String c = mem.c_str();
			split(c, ":");
			c = trim(c);
			c = split(c); // split kB
			c = trim(c);
			return toUint64(c) * 1024;
#endif
		}
		catch (...)
		{
			return 0;
		}
	}

	uint64 memoryUsed()
	{
		try
		{
#ifdef CAGE_SYSTEM_WINDOWS
			PROCESS_MEMORY_COUNTERS m;
			if (GetProcessMemoryInfo(GetCurrentProcess(), &m, sizeof(m)) == 0)
				CAGE_THROW_ERROR(SystemError, "GetProcessMemoryInfo", GetLastError());
			return m.WorkingSetSize;
#elif defined(CAGE_SYSTEM_MAC)
			// macOS doesn't have /proc, use mach task_info instead
			struct task_basic_info t_info;
			mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
			if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count) != KERN_SUCCESS)
				return 0;
			return t_info.resident_size;
#else
			std::string mem = getFile(Stringizer() + "/proc/" + currentProcessId() + "/status");
			mem = grepLine(mem, "VmRSS");
			String c = mem.c_str();
			split(c);
			c = trim(c);
			c = split(c); // split kB
			c = trim(c);
			return toUint64(c) * 1024;
#endif
		}
		catch (...)
		{
			return 0;
		}
	}
}
