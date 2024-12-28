#ifndef guard_winMain_h_jh4ser564uj
#define guard_winMain_h_jh4ser564uj

#include <cage-engine/core.h>

#ifdef _WIN32

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
		CAGE_ENGINE_API std::pair<int, const char **> winMainParams();
	}
}

int main(int argc, const char *args[]);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	const auto cmd = ::cage::privat::winMainParams();
	return ::main(cmd.first, cmd.second);
}

#endif
#endif
