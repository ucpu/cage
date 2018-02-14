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

	const skinDataStruct &widgetBaseStruct::skin() const
	{
		CAGE_ASSERT_RUNTIME(widgetState.skinIndex < base->impl->skins.size(), widgetState.skinIndex, base->impl->skins.size(), base->entity->getName());
		return base->impl->skins[widgetState.skinIndex];
	}

	renderableElementStruct *widgetBaseStruct::emitElement(elementTypeEnum element, uint32 mode, const vec4 &margin) const
	{
		vec2 pos = base->position;
		vec2 size = base->size;
		positionOffset(pos, -margin);
		sizeOffset(size, -margin);
		return emitElement(element, mode, pos, size);
	}

	renderableElementStruct *widgetBaseStruct::emitElement(elementTypeEnum element, uint32 mode, vec2 pos, vec2 size) const
	{
		CAGE_ASSERT_RUNTIME(element < elementTypeEnum::TotalElements);
		CAGE_ASSERT_RUNTIME(mode < 4);
		CAGE_ASSERT_RUNTIME(pos.valid());
		CAGE_ASSERT_RUNTIME(size.valid());
		auto *e = base->impl->emitControl;
		auto *t = e->memory.createObject<renderableElementStruct>();
		t->data.element = (uint32)element;
		t->data.mode = mode;
		t->data.outer = base->impl->pointsToNdc(pos, size);
		vec4 border = -skin().layouts[(uint32)element].border;
		positionOffset(pos, border);
		sizeOffset(size, border);
		t->data.inner = base->impl->pointsToNdc(pos, size);
		t->skinBuffer = skin().elementsGpuBuffer.get();
		t->skinTexture = skin().texture;
		e->last->next = t;
		e->last = t;
		return t;
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
