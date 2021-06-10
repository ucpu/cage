#include <cage-core/camera.h>
#include <cage-core/config.h>
#include <cage-core/files.h>
#include <cage-core/debug.h>

#include <cage-engine/engine.h>
#include <cage-engine/window.h>
#include <cage-engine/fullscreenSwitcher.h>

namespace cage
{
	namespace
	{
		string confName(const FullscreenSwitcherCreateConfig &config, const string &suffix)
		{
			if (config.configPrefix.empty())
				return suffix;
			return config.configPrefix + "/" + suffix;
		}

		class FullscreenSwitcherImpl : public FullscreenSwitcher
		{
		public:
			WindowEventListeners listeners;
			Window *window;

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
				const ivec2 p = ivec2(confWindowLeft, confWindowTop);
				const ivec2 s = ivec2(confWindowWidth, confWindowHeight);
				const bool maximize = confWindowMaximized;

				if (fullscreen)
				{
					const ivec2 s = ivec2(confFullscreenWidth, confFullscreenHeight);
					const uint32 freq = confFullscreenFrequency;
					const string screen = confScreen;
					CAGE_LOG(SeverityEnum::Info, "fullscreenSwitcher", stringizer() + "setting fullscreen window " + s + " on screen '" + screen + "'");
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

				CAGE_LOG(SeverityEnum::Info, "fullscreenSwitcher", stringizer() + "setting windowed window " + s);
				window->setWindowed();
				window->windowedPosition(p);
				window->windowedSize(s);
				if (maximize)
					window->setMaximized();
			}

			bool windowMove(const ivec2 &pos)
			{
				confScreen = window->screenId();
				if (window->isWindowed())
				{
					confWindowLeft = pos[0];
					confWindowTop = pos[1];
				}
				return false;
			}

			bool windowResize(const ivec2 &size)
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

			bool keyRelease(uint32 key, uint32, ModifiersFlags modifiers)
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

	FullscreenSwitcherCreateConfig::FullscreenSwitcherCreateConfig(bool defaultFullscreen) : defaultFullscreen(defaultFullscreen)
	{
		configPrefix = detail::configAppPrefix();
		window = cage::engineWindow();
	}

	Holder<FullscreenSwitcher> newFullscreenSwitcher(const FullscreenSwitcherCreateConfig &config)
	{
		return systemArena().createImpl<FullscreenSwitcher, FullscreenSwitcherImpl>(config);
	}
}
