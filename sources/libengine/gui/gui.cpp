#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include "private.h"

namespace cage
{
	void Gui::setOutputResolution(const ivec2 &resolution, real retina)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->outputResolution = resolution;
		impl->retina = retina;
		impl->scaling();
	}

	void Gui::setZoom(real zoom)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->zoom = zoom;
		impl->scaling();
	}

	ivec2 Gui::getOutputResolution() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->outputResolution;
	}

	real Gui::getOutputRetina() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->retina;
	}

	real Gui::getZoom() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->zoom;
	}

	void Gui::setOutputSoundBus(MixingBus *bus)
	{
		guiImpl *impl = (guiImpl*)this;
		// todo
	}

	MixingBus *Gui::getOutputSoundBus() const
	{
		guiImpl *impl = (guiImpl*)this;
		// todo
		return nullptr;
	}

	void Gui::setFocus(uint32 widget)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->focusName = widget;
		impl->focusParts = 1;
	}

	uint32 Gui::getFocus() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->focusName;
	}

	void Gui::handleWindowEvents(Window *window, sint32 order)
	{
		guiImpl *impl = (guiImpl*)this;
		impl->listeners.attachAll(window, order);
	}

	void Gui::skipAllEventsUntilNextUpdate()
	{
		guiImpl *impl = (guiImpl*)this;
		impl->eventsEnabled = false;
	}

	ivec2 Gui::getInputResolution() const
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->inputResolution;
	}

	GuiSkinConfig &Gui::skin(uint32 index)
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->skins[index];
	}

	const GuiSkinConfig &Gui::skin(uint32 index) const
	{
		const guiImpl *impl = (const guiImpl*)this;
		return impl->skins[index];
	}

	privat::GuiComponents &Gui::components()
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->components;
	}

	EntityManager *Gui::entities()
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->entityMgr.get();
	}

	AssetManager *Gui::assets()
	{
		guiImpl *impl = (guiImpl*)this;
		return impl->assetMgr;
	}

	GuiCreateConfig::GuiCreateConfig() : assetMgr(nullptr), entitiesConfig(nullptr),
		itemsArenaSize(1024 * 1024 * 16), emitArenaSize(1024 * 1024 * 16),
		skinsCount(1)
	{}

	Holder<Gui> newGui(const GuiCreateConfig &config)
	{
		return detail::systemArena().createImpl<Gui, guiImpl>(config);
	}
}
