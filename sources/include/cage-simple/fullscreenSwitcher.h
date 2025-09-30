#ifndef guard_fullscreen_switcher_h_asg5rt44j4kj456fd
#define guard_fullscreen_switcher_h_asg5rt44j4kj456fd

#include <cage-engine/core.h>

namespace cage
{
	class Window;

	class FullscreenSwitcher : private Immovable
	{
	public:
		uint32 keyToggleFullscreen = 300; // f11
		ModifiersFlags keyModifiers = ModifiersFlags::None;

		void update(bool fullscreen);
		void toggle();
		void restoreFromConfig();
	};

	namespace detail
	{
		bool defaultFullscreenSettingFromEnvironment();
	}

	struct FullscreenSwitcherCreateConfig
	{
		String configPrefix;
		Window *window = nullptr;
		bool defaultFullscreen = detail::defaultFullscreenSettingFromEnvironment();

		FullscreenSwitcherCreateConfig();
	};

	Holder<FullscreenSwitcher> newFullscreenSwitcher(const FullscreenSwitcherCreateConfig &config);
}

#endif // guard_fullscreen_switcher_h_asg5rt44j4kj456fd
