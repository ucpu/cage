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
#include <cage-core/string.h>

namespace cage
{
	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS
		bool windowsTryFile(const String &url)
		{
			if (!isPattern(toLower(url), "file:/", "", ""))
				return false;
			String file = subString(url, 0, find(url, '?'));
			file = trim(subString(file, 5, m), true, false, "/");
			char tmp[MAX_PATH + 1] = {};
			auto res = FindExecutable(file.c_str(), NULL, tmp); // find default browser
			if (res > (HINSTANCE)32)
			{
				res = ShellExecute(NULL, "open", tmp, url.c_str(), NULL, SW_SHOWNORMAL);
				return res > (HINSTANCE)32;
			}
			return false;
		}
#endif
	}

	void openUrl(const String &url)
	{
		CAGE_LOG(SeverityEnum::Info, "openUrl", Stringizer() + "opening browser with url: " + url);
#ifdef CAGE_SYSTEM_WINDOWS
		if (!windowsTryFile(url))
			ShellExecute(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
		const String s = replace(url, "\"", "\\\"");
		std::system((Stringizer() + "xdg-open \"" + s + "\"").value.c_str());
#endif
	}
}
