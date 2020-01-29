#include "../private.h"

namespace cage
{
	BaseItem::BaseItem(HierarchyItem *hierarchy) : hierarchy(hierarchy)
	{}

	BaseItem::~BaseItem()
	{}

	WidgetItem::WidgetItem(HierarchyItem *hierarchy) : BaseItem(hierarchy), skin(nullptr)
	{}

	uint32 WidgetItem::mode(bool hover, uint32 focusParts) const
	{
		if (widgetState.disabled)
			return 3;
		if (hierarchy->impl->hover == this && hover)
			return 2;
		if (hasFocus(focusParts))
			return 1;
		return 0;
	}

	uint32 WidgetItem::mode(const vec2 &pos, const vec2 &size, uint32 focusParts) const
	{
		return mode(pointInside(pos, size, hierarchy->impl->outputMouse), focusParts);
	}

	bool WidgetItem::hasFocus(uint32 parts) const
	{
		CAGE_ASSERT(hierarchy->ent);
		return hierarchy->impl->focusName && hierarchy->impl->focusName == hierarchy->ent->name() && (hierarchy->impl->focusParts & parts) > 0;
	}

	void WidgetItem::makeFocused(uint32 parts)
	{
		CAGE_ASSERT(parts != 0);
		CAGE_ASSERT(hierarchy->ent);
		CAGE_ASSERT(!widgetState.disabled);
		hierarchy->impl->focusName = hierarchy->ent->name();
		hierarchy->impl->focusParts = parts;
	}

	void WidgetItem::findFinalPosition(const FinalPosition &update)
	{
		// do nothing
	}

	void WidgetItem::generateEventReceivers()
	{
		EventReceiver e;
		e.widget = this;
		e.pos = hierarchy->renderPos;
		e.size = hierarchy->renderSize;
		if (clip(e.pos, e.size, hierarchy->clipPos, hierarchy->clipSize))
			hierarchy->impl->mouseEventReceivers.push_back(e);
	}

	RenderableElement *WidgetItem::emitElement(GuiElementTypeEnum element, uint32 mode, vec2 pos, vec2 size) const
	{
		CAGE_ASSERT(element < GuiElementTypeEnum::TotalElements);
		CAGE_ASSERT(mode < 4);
		CAGE_ASSERT(pos.valid());
		CAGE_ASSERT(size.valid());
		auto *e = hierarchy->impl->emitControl;
		auto *t = e->memory.createObject<RenderableElement>();
		t->setClip(hierarchy);
		t->data.element = (uint32)element;
		t->data.mode = mode;
		t->data.outer = hierarchy->impl->pointsToNdc(pos, size);
		vec4 border = skin->layouts[(uint32)element].border;
		offset(pos, size, -border);
		t->data.inner = hierarchy->impl->pointsToNdc(pos, size);
		t->skinBuffer = skin->elementsGpuBuffer.get();
		t->skinTexture = skin->texture;
		e->last->next = t;
		e->last = t;
		return t;
	}

	bool WidgetItem::mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point)
	{
		makeFocused();
		return true;
	}

	bool WidgetItem::mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point)
	{
		return true;
	}

	bool WidgetItem::mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point)
	{
		return true;
	}

	bool WidgetItem::mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point)
	{
		return true;
	}

	bool WidgetItem::mouseWheel(sint8 wheel, ModifiersFlags modifiers, vec2 point)
	{
		return true;
	}

	bool WidgetItem::keyPress(uint32 key, uint32 scanCode, ModifiersFlags modifiers)
	{
		return true;
	}

	bool WidgetItem::keyRepeat(uint32 key, uint32 scanCode, ModifiersFlags modifiers)
	{
		return true;
	}

	bool WidgetItem::keyRelease(uint32 key, uint32 scanCode, ModifiersFlags modifiers)
	{
		return true;
	}

	bool WidgetItem::keyChar(uint32 key)
	{
		return true;
	}
}
