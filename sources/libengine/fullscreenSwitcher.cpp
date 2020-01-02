#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/camera.h>
#include <cage-core/config.h>
#include <cage-core/files.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
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

			ConfigUint32 confWindowLeft;
			ConfigUint32 confWindowTop;
			ConfigUint32 confWindowWidth;
			ConfigUint32 confWindowHeight;
			ConfigBool confWindowMaximized;
			ConfigUint32 confFullscreenWidth;
			ConfigUint32 confFullscreenHeight;
			ConfigUint32 confFullscreenFrequency;
			ConfigString confFullscreenMonitor;
			ConfigBool confFullscreenEnabled;

			explicit FullscreenSwitcherImpl(const FullscreenSwitcherCreateConfig &config) : window(config.window),
				confWindowLeft(confName(config, "window/left"), 100),
				confWindowTop(confName(config, "window/top"), 100),
				confWindowWidth(confName(config, "window/windowWidth"), 800),
				confWindowHeight(confName(config, "window/windowHeight"), 600),
				confWindowMaximized(confName(config, "window/maximized"), true),
				confFullscreenWidth(confName(config, "window/width"), 0),
				confFullscreenHeight(confName(config, "window/height"), 0),
				confFullscreenFrequency(confName(config, "window/refreshRate"), 0),
				confFullscreenMonitor(confName(config, "window/monitor"), ""),
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
				ivec2 p = ivec2(confWindowLeft, confWindowTop);
				ivec2 s = ivec2(confWindowWidth, confWindowHeight);
				bool maximize = confWindowMaximized;

				if (fullscreen)
				{
					try
					{
						detail::OverrideBreakpoint ob;
						window->setFullscreen(ivec2(confFullscreenWidth, confFullscreenHeight), confFullscreenFrequency, confFullscreenMonitor);
						return;
					}
					catch (...)
					{
						// fall through to windowed
					}
				}

				window->setWindowed();
				window->windowedPosition(p);
				window->windowedSize(s);
				if (maximize)
					window->setMaximized();
			}

			bool windowMove(const ivec2 &pos)
			{
				if (window->isWindowed())
				{
					confWindowLeft = pos.x;
					confWindowTop = pos.y;
				}
				return false;
			}

			bool windowResize(const ivec2 &size)
			{
				if (confFullscreenEnabled = window->isFullscreen())
				{
					confFullscreenWidth = size.x;
					confFullscreenHeight = size.y;
					return false;
				}
				if (window->isWindowed())
				{
					confWindowWidth = size.x;
					confWindowHeight = size.y;
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

	FullscreenSwitcher::FullscreenSwitcher() :
		keyToggleFullscreen(300), // f11
		keyModifiers(ModifiersFlags::None)
	{}

	void FullscreenSwitcher::update(bool fullscreen)
	{
		FullscreenSwitcherImpl *impl = (FullscreenSwitcherImpl*)this;
		impl->update(fullscreen);
	}

	FullscreenSwitcherCreateConfig::FullscreenSwitcherCreateConfig(bool defaultFullscreen) : window(nullptr), defaultFullscreen(defaultFullscreen)
	{
		configPrefix = detail::getConfigAppPrefix();
		window = cage::engineWindow();
	}

	Holder<FullscreenSwitcher> newFullscreenSwitcher(const FullscreenSwitcherCreateConfig &config)
	{
		return detail::systemArena().createImpl<FullscreenSwitcher, FullscreenSwitcherImpl>(config);
	}
}
