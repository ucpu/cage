#include <cage-core/debug.h>
#include <cage-core/math.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#include <intrin.h> // __debugbreak
#else
#include <sys/types.h>
#include <sys/ptrace.h>
#endif

#include <limits>
#include <exception>
#include <atomic>
#include <cstring>

namespace cage
{
	namespace
	{
		struct IsLocal
		{
			bool breakpointEnabled = true;
			bool assertDeadly = true;
		};

		IsLocal &isLocal()
		{
			thread_local IsLocal is;
			return is;
		}

		std::atomic<bool> &isGlobalBreakpointEnabled()
		{
			static std::atomic<bool> is = { true };
			return is;
		}

		std::atomic<bool> &isGlobalAssertDeadly()
		{
			static std::atomic<bool> is = { true };
			return is;
		}

#ifndef CAGE_SYSTEM_WINDOWS
		bool isDebuggingImpl()
		{
			// another attempt, another failure
			//if (ptrace(PTRACE_TRACEME, 0, 1, 0) < 0)
			//	return true;
			// seriously, how can anyone use linux?
			return false;
		}
#endif

		bool isDebugging()
		{
#ifdef CAGE_SYSTEM_WINDOWS
			return IsDebuggerPresent();
#else
			static bool underDebugger = isDebuggingImpl();
			return underDebugger;
#endif
		}
	}

	namespace detail
	{
		void terminate()
		{
			debugOutput("invoking terminate");
			debugBreakpoint();
			std::terminate();
		}

		void debugOutput(const string &msg)
		{
			if (!isDebugging())
				return;
#ifdef CAGE_SYSTEM_WINDOWS
			string value = msg + "\n";
			OutputDebugString(value.c_str());
#else
			// todo
#endif
		}

		void debugBreakpoint()
		{
			if (!isLocal().breakpointEnabled || !isGlobalBreakpointEnabled())
				return;
			if (!isDebugging())
				return;
#ifdef CAGE_SYSTEM_WINDOWS
			__debugbreak();
#else
			__builtin_trap();
#endif
		}

		OverrideBreakpoint::OverrideBreakpoint(bool enable) : original(isLocal().breakpointEnabled)
		{
			isLocal().breakpointEnabled = enable;
		}

		OverrideBreakpoint::~OverrideBreakpoint()
		{
			isLocal().breakpointEnabled = original;
		}

		OverrideAssert::OverrideAssert(bool deadly) : original(isLocal().assertDeadly)
		{
			isLocal().assertDeadly = deadly;
		}

		OverrideAssert::~OverrideAssert()
		{
			isLocal().assertDeadly = original;
		}

		void setGlobalBreakpointOverride(bool enable)
		{
			isGlobalBreakpointEnabled() = enable;
		}

		void setGlobalAssertOverride(bool enable)
		{
			isGlobalAssertDeadly() = enable;
		}
	}

	namespace privat
	{
		namespace
		{
			void assertOutputLine(const char *msg, bool continuous = true)
			{
				if (continuous)
					CAGE_LOG_CONTINUE(SeverityEnum::Critical, "assert", msg);
				else
					CAGE_LOG(SeverityEnum::Critical, "assert", msg);
			}
		}

		void runtimeAssertFailure(const char *expt, const char *file, uintPtr line, const char *function)
		{
			char buffer[2048];
			buffer[0] = 0;
			std::strcat(buffer, "assert '");
			std::strcat(buffer, expt);
			std::strcat(buffer, "' failed");
			assertOutputLine(buffer, false);

			buffer[0] = 0;
			std::strcat(buffer, file);
			std::strcat(buffer, "(");
			char linebuf[20];
			toString(linebuf, line);
			std::strcat(buffer, linebuf);
			std::strcat(buffer, ") - ");
			std::strcat(buffer, function);
			assertOutputLine(buffer);

			if (isLocal().assertDeadly && isGlobalAssertDeadly())
				detail::terminate();
			else
				CAGE_THROW_CRITICAL(Exception, "assert failure");
		}
	}

	namespace
	{
		static_assert(sizeof(uint8) == 1, "assert size uint8");
		static_assert(sizeof(uint16) == 2, "assert size uint16");
		static_assert(sizeof(uint32) == 4, "assert size uint32");
		static_assert(sizeof(uint64) == 8, "assert size uint64");
		static_assert(sizeof(uintPtr) == sizeof(void*), "assert size uintPtr");
		static_assert(sizeof(sint8) == 1, "assert size sint8");
		static_assert(sizeof(sint16) == 2, "assert size sint16");
		static_assert(sizeof(sint32) == 4, "assert size sint32");
		static_assert(sizeof(sint64) == 8, "assert size sint64");
		static_assert(sizeof(sintPtr) == sizeof(void*), "assert size sintPtr");

		static_assert(detail::numeric_limits<char>::min() == std::numeric_limits<char>::min(), "assert numeric limits");
		static_assert(detail::numeric_limits<char>::max() == std::numeric_limits<char>::max(), "assert numeric limits");
		static_assert(detail::numeric_limits<short>::min() == std::numeric_limits<short>::min(), "assert numeric limits");
		static_assert(detail::numeric_limits<short>::max() == std::numeric_limits<short>::max(), "assert numeric limits");
		static_assert(detail::numeric_limits<int>::min() == std::numeric_limits<int>::min(), "assert numeric limits");
		static_assert(detail::numeric_limits<int>::max() == std::numeric_limits<int>::max(), "assert numeric limits");
		static_assert(detail::numeric_limits<long>::min() == std::numeric_limits<long>::min(), "assert numeric limits");
		static_assert(detail::numeric_limits<long>::max() == std::numeric_limits<long>::max(), "assert numeric limits");
		static_assert(detail::numeric_limits<long long>::min() == std::numeric_limits<long long>::min(), "assert numeric limits");
		static_assert(detail::numeric_limits<long long>::max() == std::numeric_limits<long long>::max(), "assert numeric limits");
		static_assert(detail::numeric_limits<unsigned char>::min() == std::numeric_limits<unsigned char>::min(), "assert numeric limits");
		static_assert(detail::numeric_limits<unsigned char>::max() == std::numeric_limits<unsigned char>::max(), "assert numeric limits");
		static_assert(detail::numeric_limits<unsigned short>::min() == std::numeric_limits<unsigned short>::min(), "assert numeric limits");
		static_assert(detail::numeric_limits<unsigned short>::max() == std::numeric_limits<unsigned short>::max(), "assert numeric limits");
		static_assert(detail::numeric_limits<unsigned int>::min() == std::numeric_limits<unsigned int>::min(), "assert numeric limits");
		static_assert(detail::numeric_limits<unsigned int>::max() == std::numeric_limits<unsigned int>::max(), "assert numeric limits");
		static_assert(detail::numeric_limits<unsigned long>::min() == std::numeric_limits<unsigned long>::min(), "assert numeric limits");
		static_assert(detail::numeric_limits<unsigned long>::max() == std::numeric_limits<unsigned long>::max(), "assert numeric limits");
		static_assert(detail::numeric_limits<unsigned long long>::min() == std::numeric_limits<unsigned long long>::min(), "assert numeric limits");
		static_assert(detail::numeric_limits<unsigned long long>::max() == std::numeric_limits<unsigned long long>::max(), "assert numeric limits");

		//static_assert(detail::numeric_limits<float>::min() == std::numeric_limits<float>::min(), "assert numeric limits");
		//static_assert(detail::numeric_limits<float>::max() == std::numeric_limits<float>::max(), "assert numeric limits");
		//static_assert(detail::numeric_limits<double>::min() == std::numeric_limits<double>::min(), "assert numeric limits");
		//static_assert(detail::numeric_limits<double>::max() == std::numeric_limits<double>::max(), "assert numeric limits");
		static_assert(detail::numeric_limits<float>::min() < -1.f, "assert numeric limits");
		static_assert(detail::numeric_limits<float>::max() > 1.f, "assert numeric limits");
		static_assert(detail::numeric_limits<double>::min() < -1.0, "assert numeric limits");
		static_assert(detail::numeric_limits<double>::max() > 1.0, "assert numeric limits");

		static_assert(detail::numeric_limits<uint8>::min() == std::numeric_limits<uint8>::min(), "assert numeric limits");
		static_assert(detail::numeric_limits<sint8>::max() == std::numeric_limits<sint8>::max(), "assert numeric limits");
		static_assert(detail::numeric_limits<uint16>::min() == std::numeric_limits<uint16>::min(), "assert numeric limits");
		static_assert(detail::numeric_limits<sint16>::max() == std::numeric_limits<sint16>::max(), "assert numeric limits");
		static_assert(detail::numeric_limits<uint32>::min() == std::numeric_limits<uint32>::min(), "assert numeric limits");
		static_assert(detail::numeric_limits<sint32>::max() == std::numeric_limits<sint32>::max(), "assert numeric limits");
		static_assert(detail::numeric_limits<uint64>::min() == std::numeric_limits<uint64>::min(), "assert numeric limits");
		static_assert(detail::numeric_limits<sint64>::max() == std::numeric_limits<sint64>::max(), "assert numeric limits");

		static_assert(sizeof(bool) == 1, "assert size bool");
	}
}
