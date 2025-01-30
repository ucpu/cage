#ifdef CAGE_SYSTEM_WINDOWS
	#ifndef VC_EXTRALEAN
		#define VC_EXTRALEAN
	#endif
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <windows.h> // ShellExecute
#else
	#include <cstdlib> // std::system
#endif

#include <cage-core/process.h>

namespace cage
{
	void openUrl(const String &url)
	{
#ifdef CAGE_SYSTEM_WINDOWS
		ShellExecute(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
		std::system((Stringizer() + "xdg-open " + url).value.c_str());
#endif
	}
}
