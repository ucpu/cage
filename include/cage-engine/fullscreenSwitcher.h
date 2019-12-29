#ifndef guard_fullscreen_switcher_h_asg5rt44j4kj456fd
#define guard_fullscreen_switcher_h_asg5rt44j4kj456fd

namespace cage
{
	class CAGE_API fullscreenSwitcher : private Immovable
	{
	public:
		uint32 keyToggleFullscreen;
		modifiersFlags keyModifiers;

		fullscreenSwitcher();
		void update(bool fullscreen);
	};

	struct CAGE_API fullscreenSwitcherCreateConfig
	{
		string configPrefix;
		windowHandle *window;
		bool defaultFullscreen;

		fullscreenSwitcherCreateConfig(bool defaultFullscreen = true);
	};

	CAGE_API Holder<fullscreenSwitcher> newFullscreenSwitcher(const fullscreenSwitcherCreateConfig &config);
}

#endif // guard_fullscreen_switcher_h_asg5rt44j4kj456fd
