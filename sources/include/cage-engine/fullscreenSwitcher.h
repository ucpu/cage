#ifndef guard_fullscreen_switcher_h_asg5rt44j4kj456fd
#define guard_fullscreen_switcher_h_asg5rt44j4kj456fd

#include "core.h"

namespace cage
{
	class CAGE_ENGINE_API FullscreenSwitcher : private Immovable
	{
	public:
		uint32 keyToggleFullscreen;
		ModifiersFlags keyModifiers;

		FullscreenSwitcher();
		void update(bool fullscreen);
	};

	struct CAGE_ENGINE_API FullscreenSwitcherCreateConfig
	{
		string configPrefix;
		Window *window;
		bool defaultFullscreen;

		FullscreenSwitcherCreateConfig(bool defaultFullscreen = true);
	};

	CAGE_ENGINE_API Holder<FullscreenSwitcher> newFullscreenSwitcher(const FullscreenSwitcherCreateConfig &config);
}

#endif // guard_fullscreen_switcher_h_asg5rt44j4kj456fd
