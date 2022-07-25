#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace
{
	struct AutoHandle : private cage::Immovable
	{
		HANDLE handle = 0;

		void close() noexcept
		{
			if (handle)
			{
				::CloseHandle(handle);
				handle = 0;
			}
		}

		~AutoHandle()
		{
			close();
		}
	};
}
