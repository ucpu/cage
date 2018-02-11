#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
#include "private.h"

namespace cage
{
	void guiClass::setOutputResolution(const pointStruct &resolution, real retina)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->outputResolution = resolution;
		impl->retina = retina;
		impl->scaling();
	}

	void guiClass::setZoom(real zoom)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->zoom = zoom;
		impl->scaling();
	}

	pointStruct guiClass::getOutputResolution() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->outputResolution;
	}

	real guiClass::getOutputRetina() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->retina;
	}

	real guiClass::getZoom() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->zoom;
	}

	void guiClass::setOutputSoundBus(busClass *bus)
	{
		guiImpl *impl = (guiImpl*)this;
		// todo
	}

	busClass *guiClass::getOutputSoundBus() const
	{
		guiImpl *impl = (guiImpl*)this;
		// todo
		return nullptr;
	}

	void guiClass::setFocus(uint32 widget)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->focusName = widget;
	}

	uint32 guiClass::getFocus() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->focusName;
	}

	void guiClass::handleWindowEvents(windowClass *window)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->listeners.attachAll(window);
	}

	void guiClass::skipAllEventsUntilNextUpdate()
	{
		guiImpl *impl = (guiImpl*)this;
		impl->eventsEnabled = false;
	}

	pointStruct guiClass::getInputResolution() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->inputResolution;
	}

	skinConfigStruct &guiClass::skin(uint32 index)
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->skins[index];
	}

	const skinConfigStruct &guiClass::skin(uint32 index) const
	{
		const guiImpl *impl = (const guiImpl*)this;
		return impl->skins[index];
	}

	componentsStruct &guiClass::components()
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->components;
	}

	entityManagerClass *guiClass::entities()
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->entityManager.get();
	}

	assetManagerClass *guiClass::assets()
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->assetManager;
	}

	guiCreateConfig::guiCreateConfig() : assetManager(nullptr), entitiesConfig(nullptr),
		itemsArenaSize(1024 * 1024 * 16), emitArenaSize(1024 * 1024 * 16),
		skinsCount(1)
	{}

	holder<guiClass> newGui(const guiCreateConfig &config)
	{
		return detail::systemArena().createImpl<guiClass, guiImpl>(config);
	}
}
