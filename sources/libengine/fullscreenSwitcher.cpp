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
		string confName(const fullscreenSwitcherCreateConfig &config, const string &suffix)
		{
			if (config.configPrefix.empty())
				return suffix;
			return config.configPrefix + "." + suffix;
		}

		class fullscreenSwitcherImpl : public fullscreenSwitcher, public fullscreenSwitcherCreateConfig
		{
		public:
			windowEventListeners listeners;
			windowHandle *window;

			configUint32 confWindowLeft;
			configUint32 confWindowTop;
			configUint32 confWindowWidth;
			configUint32 confWindowHeight;
			configUint32 confFullscreenWidth;
			configUint32 confFullscreenHeight;
			configBool confFullscreenEnabled;

			fullscreenSwitcherImpl(const fullscreenSwitcherCreateConfig &config) : window(config.window),
				confWindowLeft(confName(config, "window.left"), 100),
				confWindowTop(confName(config, "window.top"), 100),
				confWindowWidth(confName(config, "window.width"), 0),
				confWindowHeight(confName(config, "window.height"), 0),
				confFullscreenWidth(confName(config, "fullscreen.width"), 0),
				confFullscreenHeight(confName(config, "fullscreen.height"), 0),
				confFullscreenEnabled(confName(config, "fullscreen.enabled"), config.defaultFullscreen)
			{
				CAGE_ASSERT(window);
				listeners.attachAll(window);
				listeners.windowMove.bind<fullscreenSwitcherImpl, &fullscreenSwitcherImpl::windowMove>(this);
				listeners.windowResize.bind<fullscreenSwitcherImpl, &fullscreenSwitcherImpl::windowResize>(this);
				listeners.keyRelease.bind<fullscreenSwitcherImpl, &fullscreenSwitcherImpl::keyRelease>(this);
				if (window->isHidden())
					update(confFullscreenEnabled);
			}

			void update(bool fullscreen)
			{
				if (fullscreen)
				{
					try
					{
						detail::overrideBreakpoint ob;
						window->setFullscreen(ivec2(confFullscreenWidth, confFullscreenHeight));
						confFullscreenEnabled = true;
					}
					catch (...)
					{
						update(false);
					}
				}
				else
				{
					confFullscreenEnabled = false;
					if (confWindowWidth == 0 || confWindowHeight == 0)
						window->setMaximized();
					else
					{
						window->setWindowed();
						window->windowedSize(ivec2(confWindowWidth, confWindowHeight));
						window->windowedPosition(ivec2(confWindowLeft, confWindowTop));
					}
				}
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
				if (window->isWindowed())
				{
					confWindowWidth = size.x;
					confWindowHeight = size.y;
				}
				else if (window->isMaximized())
				{
					confWindowWidth = 0;
					confWindowHeight = 0;
				}
				return false;
			}

			bool keyRelease(uint32 key, uint32, modifiersFlags modifiers)
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

	fullscreenSwitcher::fullscreenSwitcher() :
		keyToggleFullscreen(300), // f11
		keyModifiers(modifiersFlags::None)
	{}

	void fullscreenSwitcher::update(bool fullscreen)
	{
		fullscreenSwitcherImpl *impl = (fullscreenSwitcherImpl*)this;
		impl->update(fullscreen);
	}

	fullscreenSwitcherCreateConfig::fullscreenSwitcherCreateConfig() : window(nullptr), defaultFullscreen(true)
	{
		configPrefix = detail::getConfigAppPrefix();
		window = cage::window();
	}

	holder<fullscreenSwitcher> newFullscreenSwitcher(const fullscreenSwitcherCreateConfig &config)
	{
		return detail::systemArena().createImpl<fullscreenSwitcher, fullscreenSwitcherImpl>(config);
	}
}
