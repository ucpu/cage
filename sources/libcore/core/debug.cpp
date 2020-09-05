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
			toString(linebuf, 20, line);
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
		static_assert(sizeof(bool) == 1, "assert size bool");

		static_assert(std::is_trivially_copy_constructible<string>::value, "string not trivial");
		static_assert(std::is_trivially_move_constructible<string>::value, "string not trivial");
		static_assert(std::is_trivially_copy_assignable<string>::value, "string not trivial");
		static_assert(std::is_trivially_move_assignable<string>::value, "string not trivial");
		static_assert(std::is_trivially_copyable<string>::value, "string not trivial");
		static_assert(std::is_trivially_destructible<string>::value, "string not trivial");
	}
}
