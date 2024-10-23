#ifdef _WIN32

	#include <cage-engine/core.h>

	#ifndef VC_EXTRALEAN
		#define VC_EXTRALEAN
	#endif
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <windows.h>

namespace cage
{
	namespace privat
	{
		CAGE_API_EXPORT std::pair<int, const char **> winMainParams()
		{
			using namespace cage;

			CAGE_LOG(SeverityEnum::Info, "cage-engine", "winMain parsing parameters");

			static int consoleDummy = []()
			{
				AllocConsole(); // allocate new console for sub-programs
				ShowWindow(GetConsoleWindow(), SW_HIDE);
				return 0;
			}();
			(void)consoleDummy;

			LPWSTR cmdLine = GetCommandLineW();
			int argc;
			LPWSTR *argvW = CommandLineToArgvW(cmdLine, &argc);

			if (argvW == nullptr)
			{
				static_assert(sizeof(DWORD) == sizeof(uint32));
				CAGE_LOG_THROW(Stringizer() + "error code: " + (uint32)GetLastError());
				CAGE_THROW_CRITICAL(Exception, "failed to convert command line arguments to an array (CommandLineToArgvW)");
			}

			// convert wide char to multibyte
			char **argv = new char *[argc];
			for (int i = 0; i < argc; i++)
			{
				size_t len = wcslen(argvW[i]) + 1;
				argv[i] = new char[len];
				wcstombs(argv[i], argvW[i], len);
			}

			return { argc, (const char **)argv };
		}
	}
}

#endif
