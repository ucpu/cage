#ifndef guard_clipboard_h_sfghwhjshjsks4ds65
#define guard_clipboard_h_sfghwhjshjsks4ds65

#include <cage-engine/core.h>

namespace cage
{
	CAGE_ENGINE_API void setClipboard(const char *str);
	CAGE_ENGINE_API void setClipboardString(const String &str);
	CAGE_ENGINE_API PointerRange<const char> getClipboard();
	CAGE_ENGINE_API String getClipboardString();
}

#endif
