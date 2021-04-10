#include "private.h"

namespace cage
{
	void Gui::setOutputResolution(const ivec2 &resolution, real retina)
	{
		GuiImpl *impl = (GuiImpl*)this;
		impl->outputResolution = resolution;
		impl->retina = retina;
		impl->scaling();
	}

	void Gui::setZoom(real zoom)
	{
		GuiImpl *impl = (GuiImpl*)this;
		impl->zoom = zoom;
		impl->scaling();
	}

	ivec2 Gui::getOutputResolution() const
	{
		GuiImpl *impl = (GuiImpl*)this;
		return impl->outputResolution;
	}

	real Gui::getOutputRetina() const
	{
		GuiImpl *impl = (GuiImpl*)this;
		return impl->retina;
	}

	real Gui::getZoom() const
	{
		GuiImpl *impl = (GuiImpl*)this;
		return impl->zoom;
	}

	void Gui::setFocus(uint32 widget)
	{
		GuiImpl *impl = (GuiImpl*)this;
		impl->focusName = widget;
		impl->focusParts = 1;
	}

	uint32 Gui::getFocus() const
	{
		GuiImpl *impl = (GuiImpl*)this;
		return impl->focusName;
	}

	void Gui::handleWindowEvents(Window *window, sint32 order)
	{
		GuiImpl *impl = (GuiImpl*)this;
		impl->listeners.attachAll(window, order);
	}

	void Gui::skipAllEventsUntilNextUpdate()
	{
		GuiImpl *impl = (GuiImpl*)this;
		impl->eventsEnabled = false;
	}

	ivec2 Gui::getInputResolution() const
	{
		GuiImpl *impl = (GuiImpl*)this;
		return impl->inputResolution;
	}

	GuiSkinConfig &Gui::skin(uint32 index)
	{
		GuiImpl *impl = (GuiImpl*)this;
		return impl->skins[index];
	}

	const GuiSkinConfig &Gui::skin(uint32 index) const
	{
		const GuiImpl *impl = (const GuiImpl*)this;
		return impl->skins[index];
	}

	privat::GuiComponents &Gui::components()
	{
		GuiImpl *impl = (GuiImpl*)this;
		return impl->components;
	}

	EntityManager *Gui::entities()
	{
		GuiImpl *impl = (GuiImpl*)this;
		return impl->entityMgr.get();
	}

	AssetManager *Gui::assets()
	{
		GuiImpl *impl = (GuiImpl*)this;
		return impl->assetMgr;
	}

	Holder<Gui> newGui(const GuiCreateConfig &config)
	{
		return systemArena().createImpl<Gui, GuiImpl>(config);
	}
}
