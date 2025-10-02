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
		void windowsOpenUrl(const String &url)
		{
			const String esc = Stringizer() + "\"" + url + "\"";
			if (isPattern(toLower(url), "file:/", "", ""))
			{
				String file = subString(url, 0, find(url, '?'));
				file = trim(subString(file, 5, m), true, false, "/");
				char tmp[MAX_PATH + 1] = {};
				auto res = FindExecutable(file.c_str(), NULL, tmp); // find default browser
				if (res > (HINSTANCE)32)
				{
					res = ShellExecute(NULL, "open", tmp, esc.c_str(), NULL, SW_SHOWNORMAL);
					if (res > (HINSTANCE)32)
						return;
				}
			}
			ShellExecute(NULL, "open", (Stringizer() + "\"" + url + "\"").value.c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
#endif
	}

	void openUrl(const String &url_)
	{
		const String url = replace(url_, "\"", "\\\"");
		CAGE_LOG(SeverityEnum::Info, "openUrl", Stringizer() + "opening browser with url: " + url);
#ifdef CAGE_SYSTEM_WINDOWS
		windowsOpenUrl(url);
#else
		std::system((Stringizer() + "xdg-open \"" + url + "\"").value.c_str());
#endif
	}
}
