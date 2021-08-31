#include <cage-core/camera.h>
#include <cage-core/config.h>
#include <cage-core/files.h>
#include <cage-core/debug.h>
#include <cage-core/string.h>

#include <cage-engine/engine.h>
#include <cage-engine/window.h>
#include <cage-engine/fullscreenSwitcher.h>

#include <cstdlib>

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
			WindowEventListeners listeners;
			Window *window = nullptr;

			ConfigSint32 confWindowLeft;
			ConfigSint32 confWindowTop;
			ConfigUint32 confWindowWidth;
			ConfigUint32 confWindowHeight;
			ConfigBool confWindowMaximized;
			ConfigUint32 confFullscreenWidth;
			ConfigUint32 confFullscreenHeight;
			ConfigUint32 confFullscreenFrequency;
			ConfigString confScreen;
			ConfigBool confFullscreenEnabled;

			explicit FullscreenSwitcherImpl(const FullscreenSwitcherCreateConfig &config) : window(config.window),
				confWindowLeft(confName(config, "window/left"), 100),
				confWindowTop(confName(config, "window/top"), 100),
				confWindowWidth(confName(config, "window/windowWidth"), 800),
				confWindowHeight(confName(config, "window/windowHeight"), 600),
				confWindowMaximized(confName(config, "window/maximized"), true),
				confScreen(confName(config, "window/screen"), ""),
				confFullscreenWidth(confName(config, "window/width"), 0),
				confFullscreenHeight(confName(config, "window/height"), 0),
				confFullscreenFrequency(confName(config, "window/refreshRate"), 0),
				confFullscreenEnabled(confName(config, "window/fullscreen"), config.defaultFullscreen)
			{
				CAGE_ASSERT(window);
				listeners.attachAll(window);
				listeners.windowMove.bind<FullscreenSwitcherImpl, &FullscreenSwitcherImpl::windowMove>(this);
				listeners.windowResize.bind<FullscreenSwitcherImpl, &FullscreenSwitcherImpl::windowResize>(this);
				listeners.keyRelease.bind<FullscreenSwitcherImpl, &FullscreenSwitcherImpl::keyRelease>(this);
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
					CAGE_LOG(SeverityEnum::Info, "fullscreenSwitcher", Stringizer() + "setting fullscreen window " + s + " on screen '" + screen + "'");
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

				CAGE_LOG(SeverityEnum::Info, "fullscreenSwitcher", Stringizer() + "setting windowed window " + s);
				window->setWindowed();
				window->windowedPosition(p);
				window->windowedSize(s);
				if (maximize)
					window->setMaximized();
			}

			bool windowMove(const Vec2i &pos)
			{
				confScreen = window->screenId();
				if (window->isWindowed())
				{
					confWindowLeft = pos[0];
					confWindowTop = pos[1];
				}
				return false;
			}

			bool windowResize(const Vec2i &size)
			{
				confScreen = window->screenId();
				if ((confFullscreenEnabled = window->isFullscreen()))
				{
					confFullscreenWidth = size[0];
					confFullscreenHeight = size[1];
					return false;
				}
				if (window->isWindowed())
				{
					confWindowWidth = size[0];
					confWindowHeight = size[1];
				}
				confWindowMaximized = window->isMaximized();
				return false;
			}

			bool keyRelease(uint32 key, ModifiersFlags modifiers)
			{
				if (modifiers == keyModifiers && key == keyToggleFullscreen)
				{
					update(!window->isFullscreen());
					return true;
				}
				return false;
			}
		};
	}

	void FullscreenSwitcher::update(bool fullscreen)
	{
		FullscreenSwitcherImpl *impl = (FullscreenSwitcherImpl*)this;
		impl->update(fullscreen);
	}

	namespace
	{
		bool defaultFullscreenEnvironment()
		{
			const char *env = std::getenv("CAGE_FULLSCREEN_DEFAULT");
			if (!env)
				return true;
			try
			{
				const bool r = toBool(trim(String(env)));
				CAGE_LOG(SeverityEnum::Warning, "fullscreenSwitcher", Stringizer() + "using environment variable CAGE_FULLSCREEN_DEFAULT, value: " + r);
				return r;
			}
			catch (const Exception &)
			{
				CAGE_LOG(SeverityEnum::Warning, "fullscreenSwitcher", "failed parsing environment variable CAGE_FULLSCREEN_DEFAULT");
			}
			return false;
		}
	}

	FullscreenSwitcherCreateConfig::FullscreenSwitcherCreateConfig() : FullscreenSwitcherCreateConfig(defaultFullscreenEnvironment())
	{}

	FullscreenSwitcherCreateConfig::FullscreenSwitcherCreateConfig(bool defaultFullscreen) : defaultFullscreen(defaultFullscreen)
	{
		configPrefix = detail::globalConfigPrefix();
		window = cage::engineWindow();
	}

	Holder<FullscreenSwitcher> newFullscreenSwitcher(const FullscreenSwitcherCreateConfig &config)
	{
		return systemMemory().createImpl<FullscreenSwitcher, FullscreenSwitcherImpl>(config);
	}
}
