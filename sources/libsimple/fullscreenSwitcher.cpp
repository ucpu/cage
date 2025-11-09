#include <cstdlib> // std::getenv

#include <cage-core/config.h>
#include <cage-core/debug.h>
#include <cage-core/string.h>
#include <cage-engine/window.h>
#include <cage-simple/engine.h>
#include <cage-simple/fullscreenSwitcher.h>

namespace cage
{
	namespace
	{
		String confName(const FullscreenSwitcherCreateConfig &config, const String &suffix)
		{
			if (config.configPrefix.empty())
				return suffix;
			return config.configPrefix + "/" + suffix;
		}

		class FullscreenSwitcherImpl : public FullscreenSwitcher
		{
		public:
			Window *const window = nullptr;
			const EventListener<bool(const GenericInput &)> windowMoveListener = window->events.listen(inputFilter([this](input::WindowMove in) { return this->windowMove(in); }), -13665);
			const EventListener<bool(const GenericInput &)> windowResizeListener = window->events.listen(inputFilter([this](input::WindowResize in) { return this->windowResize(in); }), -13666);
			const EventListener<bool(const GenericInput &)> keyListener = window->events.listen(inputFilter([this](input::KeyRelease in) { return this->keyRelease(in); }), -13667);

			ConfigBool confFullscreenEnabled;
			ConfigBool confWindowMaximized;
			ConfigSint32 confWindowLeft;
			ConfigSint32 confWindowTop;
			ConfigUint32 confWindowWidth;
			ConfigUint32 confWindowHeight;
			ConfigUint32 confFullscreenWidth;
			ConfigUint32 confFullscreenHeight;
			ConfigUint32 confFullscreenFrequency;
			ConfigString confScreen;

			explicit FullscreenSwitcherImpl(const FullscreenSwitcherCreateConfig &config) : window(config.window), confFullscreenEnabled(confName(config, "window/fullscreen"), config.defaultFullscreen), confWindowMaximized(confName(config, "window/maximized"), true), confWindowLeft(confName(config, "window/left"), 100), confWindowTop(confName(config, "window/top"), 100), confWindowWidth(confName(config, "window/windowWidth"), 800), confWindowHeight(confName(config, "window/windowHeight"), 600), confFullscreenWidth(confName(config, "window/width"), 0), confFullscreenHeight(confName(config, "window/height"), 0), confFullscreenFrequency(confName(config, "window/refreshRate"), 0), confScreen(confName(config, "window/screen"), "")
			{
				CAGE_ASSERT(window);
				if (window->isHidden())
					update(confFullscreenEnabled);
			}

			void update(bool fullscreen)
			{
				const Vec2i p = Vec2i(confWindowLeft, confWindowTop);
				const Vec2i s = Vec2i(confWindowWidth, confWindowHeight);
				const bool maximize = confWindowMaximized;

				if (fullscreen)
				{
					const Vec2i s = Vec2i(confFullscreenWidth, confFullscreenHeight);
					const uint32 freq = confFullscreenFrequency;
					const String screen = confScreen;
					CAGE_LOG(SeverityEnum::Info, "fullscreenSwitcher", Stringizer() + "setting fullscreen window: " + s + ", on screen: " + screen);
					try
					{
						detail::OverrideBreakpoint ob;
						window->setFullscreen(s, freq, screen);
						return;
					}
					catch (...)
					{
						// fall through to windowed
					}
				}

				CAGE_LOG(SeverityEnum::Info, "fullscreenSwitcher", Stringizer() + "setting windowed window: " + s);
				window->setWindowed();
				window->windowedPosition(p);
				window->windowedSize(s);
				if (maximize)
					window->setMaximized();
			}

			void windowMove(input::WindowMove in)
			{
				confScreen = window->screenId();
				if (window->isWindowed())
				{
					confWindowLeft = in.position[0];
					confWindowTop = in.position[1];
				}
			}

			void windowResize(input::WindowResize in)
			{
				confScreen = window->screenId();
				if ((confFullscreenEnabled = window->isFullscreen()))
				{
					confFullscreenWidth = in.size[0];
					confFullscreenHeight = in.size[1];
					return;
				}
				if (window->isWindowed())
				{
					confWindowWidth = in.size[0];
					confWindowHeight = in.size[1];
				}
				confWindowMaximized = window->isMaximized();
			}

			bool keyRelease(input::KeyRelease in)
			{
				if (in.mods == keyModifiers && in.key == keyToggleFullscreen)
				{
					toggle();
					return true;
				}
				return false;
			}
		};
	}

	void FullscreenSwitcher::update(bool fullscreen)
	{
		FullscreenSwitcherImpl *impl = (FullscreenSwitcherImpl *)this;
		impl->update(fullscreen);
	}

	void FullscreenSwitcher::toggle()
	{
		FullscreenSwitcherImpl *impl = (FullscreenSwitcherImpl *)this;
		update(!impl->window->isFullscreen());
	}

	void FullscreenSwitcher::restoreFromConfig()
	{
		FullscreenSwitcherImpl *impl = (FullscreenSwitcherImpl *)this;
		impl->update(impl->confFullscreenEnabled);
	}

	namespace detail
	{
		bool defaultFullscreenSettingFromEnvironment()
		{
			const char *env = std::getenv("CAGE_FULLSCREEN_DEFAULT");
			if (!env)
				return true;
			try
			{
				const bool r = toBool(trim(String(env)));
				CAGE_LOG(SeverityEnum::Info, "fullscreenSwitcher", Stringizer() + "using environment variable CAGE_FULLSCREEN_DEFAULT, value: " + r);
				return r;
			}
			catch (const Exception &)
			{
				CAGE_LOG(SeverityEnum::Warning, "fullscreenSwitcher", "failed parsing environment variable CAGE_FULLSCREEN_DEFAULT");
			}
#ifdef CAGE_SYSTEM_MAC
			return false;
#else
			return true;
#endif
		}
	}

	FullscreenSwitcherCreateConfig::FullscreenSwitcherCreateConfig()
	{
		configPrefix = detail::globalConfigPrefix();
		window = cage::engineWindow();
	}

	Holder<FullscreenSwitcher> newFullscreenSwitcher(const FullscreenSwitcherCreateConfig &config)
	{
		return systemMemory().createImpl<FullscreenSwitcher, FullscreenSwitcherImpl>(config);
	}
}
