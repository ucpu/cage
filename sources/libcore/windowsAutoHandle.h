#ifndef guard_windowsAutoHandle_h_d5t4hftg85fgujkki
#define guard_windowsAutoHandle_h_d5t4hftg85fgujkki

#include <windows.h>

#include <cage-core/core.h>

namespace cage
{
	struct AutoHandle : private Immovable
	{
		HANDLE handle = 0;

		void close()
		{
			if (handle)
			{
				::CloseHandle(handle);
				handle = 0;
			}
		}

		~AutoHandle() { close(); }
	};
}

#endif
