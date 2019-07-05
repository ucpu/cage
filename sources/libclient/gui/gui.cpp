#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "private.h"

namespace cage
{
	void guiManager::setOutputResolution(const ivec2 &resolution, real retina)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->outputResolution = resolution;
		impl->retina = retina;
		impl->scaling();
	}

	void guiManager::setZoom(real zoom)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->zoom = zoom;
		impl->scaling();
	}

	ivec2 guiManager::getOutputResolution() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->outputResolution;
	}

	real guiManager::getOutputRetina() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->retina;
	}

	real guiManager::getZoom() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->zoom;
	}

	void guiManager::setOutputSoundBus(mixingBus *bus)
	{
		guiImpl *impl = (guiImpl*)this;
		// todo
	}

	mixingBus *guiManager::getOutputSoundBus() const
	{
		guiImpl *impl = (guiImpl*)this;
		// todo
		return nullptr;
	}

	void guiManager::setFocus(uint32 widget)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->focusName = widget;
		impl->focusParts = 1;
	}

	uint32 guiManager::getFocus() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->focusName;
	}

	void guiManager::handleWindowEvents(windowHandle *window, sint32 order)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->listeners.attachAll(window, order);
	}

	void guiManager::skipAllEventsUntilNextUpdate()
	{
		guiImpl *impl = (guiImpl*)this;
		impl->eventsEnabled = false;
	}

	ivec2 guiManager::getInputResolution() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->inputResolution;
	}

	guiSkinConfig &guiManager::skin(uint32 index)
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->skins[index];
	}

	const guiSkinConfig &guiManager::skin(uint32 index) const
	{
		const guiImpl *impl = (const guiImpl*)this;
		return impl->skins[index];
	}

	guiComponents &guiManager::components()
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->components;
	}

	entityManager *guiManager::entities()
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->entityMgr.get();
	}

	assetManager *guiManager::assets()
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->assetMgr;
	}

	guiManagerCreateConfig::guiManagerCreateConfig() : assetMgr(nullptr), entitiesConfig(nullptr),
		itemsArenaSize(1024 * 1024 * 16), emitArenaSize(1024 * 1024 * 16),
		skinsCount(1)
	{}

	holder<guiManager> newGuiManager(const guiManagerCreateConfig &config)
	{
		return detail::systemArena().createImpl<guiManager, guiImpl>(config);
	}
}
