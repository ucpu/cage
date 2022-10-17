#include <cage-core/debug.h>
#include <cage-core/string.h>

#if !defined(CAGE_CORE_API)
#error CAGE_CORE_API must be defined
#endif
#if defined(CAGE_ARCHITECTURE_64) == defined(CAGE_ARCHITECTURE_32)
#error exactly one of CAGE_ARCHITECTURE_64 and CAGE_ARCHITECTURE_32 must be defined
#endif
#if defined(CAGE_DEBUG) == defined(NDEBUG)
#error exactly one of CAGE_DEBUG and NDEBUG must be defined
#endif
#if defined(CAGE_DEBUG) && !defined(CAGE_ASSERT_ENABLED)
#error CAGE_ASSERT_ENABLED must be defined for debug builds
#endif
#if defined(CAGE_DEBUG)
static_assert(CAGE_DEBUG_BOOL == true);
#else
static_assert(CAGE_DEBUG_BOOL == false);
#endif

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#include <intrin.h> // __debugbreak
#endif

#include <atomic>
#include <exception> // std::terminate
#include <cstring> // std::strcat
#include <fstream> // std::ifstream
#include <string> // std::getline

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
			static std::atomic<bool> is = true;
			return is;
		}

		std::atomic<bool> &isGlobalAssertDeadly()
		{
			static std::atomic<bool> is = true;
			return is;
		}
	}

	namespace detail
	{
		bool isDebugging()
		{
#ifdef CAGE_SYSTEM_WINDOWS
			return IsDebuggerPresent();
#else
			static bool underDebugger = []() {
				try
				{
					FILE *f = std::fopen("/proc/self/status", "r");
					if (!f)
						return false;
					struct Closer { FILE *f = nullptr; Closer(FILE *f) : f(f) {} ~Closer() { std::fclose(f); } } closer(f);
					String s;
					while (std::fgets(s.rawData(), s.MaxLength, f))
					{
						s.rawLength() = std::strlen(s.rawData());
						if (!isPattern(s, "TracerPid:", "", ""))
							continue;
						s = trim(remove(s, 0, 11));
						return toSint64(s) != 0;
					}
					return false;
				}
				catch (...)
				{
					return false;
				}
			}();
			return underDebugger;
#endif
		}

		void terminate()
		{
			debugOutput("invoking terminate");
			debugBreakpoint();
			std::terminate();
		}

		void debugOutput(const String &msg)
		{
			if (!isDebugging())
				return;
#ifdef CAGE_SYSTEM_WINDOWS
			String value = msg + "\n";
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
#if defined(_MSC_VER)
			__debugbreak();
#elif defined(__clang__)
			__builtin_debugtrap();
#else
			__asm__ volatile("int $0x03");
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

		void globalBreakpointOverride(bool enable)
		{
			isGlobalBreakpointEnabled() = enable;
		}

		void globalAssertOverride(bool enable)
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

		void runtimeAssertFailure(const std::source_location &location, StringLiteral expt)
		{
			char buffer[2048];
			buffer[0] = 0;
			std::strcat(buffer, "assert '");
			std::strcat(buffer, expt);
			std::strcat(buffer, "' failed");
			assertOutputLine(buffer, false);

			buffer[0] = 0;
			std::strcat(buffer, location.file_name());
			std::strcat(buffer, "(");
			char linebuf[20];
			toString(linebuf, 20, location.line());
			std::strcat(buffer, linebuf);
			std::strcat(buffer, ") - ");
			std::strcat(buffer, location.function_name());
			assertOutputLine(buffer);

			if (isLocal().assertDeadly && isGlobalAssertDeadly())
				detail::terminate();
			else
			{
				auto e = Exception(location, ::cage::SeverityEnum::Critical, "assert failure");
				e.makeLog();
				throw e;
			}
		}
	}

	namespace
	{
		static_assert(sizeof(uint8) == 1);
		static_assert(sizeof(uint16) == 2);
		static_assert(sizeof(uint32) == 4);
		static_assert(sizeof(uint64) == 8);
		static_assert(sizeof(uintPtr) == sizeof(void*));
		static_assert(sizeof(sint8) == 1);
		static_assert(sizeof(sint16) == 2);
		static_assert(sizeof(sint32) == 4);
		static_assert(sizeof(sint64) == 8);
		static_assert(sizeof(sintPtr) == sizeof(void*));
		static_assert(sizeof(bool) == 1);

		static_assert(std::is_trivially_copy_constructible_v<String>);
		static_assert(std::is_trivially_move_constructible_v<String>);
		static_assert(std::is_trivially_copy_assignable_v<String>);
		static_assert(std::is_trivially_move_assignable_v<String>);
		static_assert(std::is_trivially_copyable_v<String>);
		static_assert(std::is_trivially_destructible_v<String>);

		static_assert(sizeof(String) == 1024);
	}
}
