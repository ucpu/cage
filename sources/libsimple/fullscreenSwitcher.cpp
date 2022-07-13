#include <cage-core/camera.h>
#include <cage-core/config.h>
#include <cage-core/files.h>
#include <cage-core/debug.h>
#include <cage-core/string.h>
#include <cage-engine/window.h>

#include <cage-simple/engine.h>
#include <cage-simple/fullscreenSwitcher.h>

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
			InputListener<InputClassEnum::WindowMove, InputWindowValue> windowMoveListener;
			InputListener<InputClassEnum::WindowResize, InputWindowValue> windowResizeListener;
			InputListener<InputClassEnum::KeyRelease, InputKey, bool> keyListener;
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
				windowMoveListener.attach(window->events, -13665);
				windowMoveListener.bind<FullscreenSwitcherImpl, &FullscreenSwitcherImpl::windowMove>(this);
				windowResizeListener.attach(window->events, -13666);
				windowResizeListener.bind<FullscreenSwitcherImpl, &FullscreenSwitcherImpl::windowResize>(this);
				keyListener.attach(window->events, -13667);
				keyListener.bind<FullscreenSwitcherImpl, &FullscreenSwitcherImpl::keyRelease>(this);
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

			void windowMove(InputWindowValue in)
			{
				confScreen = window->screenId();
				if (window->isWindowed())
				{
					confWindowLeft = in.value[0];
					confWindowTop = in.value[1];
				}
			}

			void windowResize(InputWindowValue in)
			{
				confScreen = window->screenId();
				if ((confFullscreenEnabled = window->isFullscreen()))
				{
					confFullscreenWidth = in.value[0];
					confFullscreenHeight = in.value[1];
					return;
				}
				if (window->isWindowed())
				{
					confWindowWidth = in.value[0];
					confWindowHeight = in.value[1];
				}
				confWindowMaximized = window->isMaximized();
			}

			bool keyRelease(InputKey in)
			{
				if (in.mods == keyModifiers && in.key == keyToggleFullscreen)
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
