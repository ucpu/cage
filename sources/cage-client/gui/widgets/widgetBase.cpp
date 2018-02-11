#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
#include "../private.h"

namespace cage
{
	widgetBaseStruct::widgetBaseStruct(guiItemStruct *base) : base(base)
	{
		auto *impl = base->impl;
		if (GUI_HAS_COMPONENT(widgetState, base->entity))
		{
			GUI_GET_COMPONENT(widgetState, ws, base->entity);
			widgetState = ws;
		}
	}

	const skinConfigStruct &widgetBaseStruct::skin() const
	{
		CAGE_ASSERT_RUNTIME(widgetState.skinIndex < base->impl->skins.size(), widgetState.skinIndex, base->impl->skins.size(), base->entity->getName());
		return base->impl->skin(widgetState.skinIndex);
	}

	bool widgetBaseStruct::mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool widgetBaseStruct::mouseDouble(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool widgetBaseStruct::mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool widgetBaseStruct::mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool widgetBaseStruct::mouseWheel(sint8 wheel, modifiersFlags modifiers, vec2 point)
	{
		return false;
	}

	bool widgetBaseStruct::keyPress(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		return false;
	}

	bool widgetBaseStruct::keyRepeat(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		return false;
	}

	bool widgetBaseStruct::keyRelease(uint32 key, uint32 scanCode, modifiersFlags modifiers)
	{
		return false;
	}

	bool widgetBaseStruct::keyChar(uint32 key)
	{
		return false;
	}
}
