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
	updatePositionStruct::updatePositionStruct() : position(vec2::Nan), size(vec2::Nan)
	{}

	guiItemStruct::guiItemStruct(guiImpl *impl, entityClass *entity) : impl(impl), entity(entity),
		requestedSize(vec2::Nan), position(vec2::Nan), size(vec2::Nan), contentPosition(vec2::Nan), contentSize(vec2::Nan),
		parent(nullptr), prevSibling(nullptr), nextSibling(nullptr), firstChild(nullptr), lastChild(nullptr), order(0),
		widget(nullptr), layout(nullptr), text(nullptr), image(nullptr)
	{}

	void guiItemStruct::initialize()
	{
		if (widget)
			widget->initialize();
		if (layout)
			layout->initialize();
		if (text)
			text->initialize();
		if (image)
			image->initialize();
	}

	void guiItemStruct::updateRequestedSize()
	{
		if (widget)
			widget->updateRequestedSize();
		else if (layout)
			layout->updateRequestedSize();
		else if (text)
			text->updateRequestedSize();
		else if (image)
			image->updateRequestedSize();
	}

	void guiItemStruct::updateFinalPosition(const updatePositionStruct &update)
	{
		position = update.position;
		size = update.size;
		if (widget)
			widget->updateFinalPosition(update);
		else
		{
			CAGE_ASSERT_RUNTIME(layout, "trying to layout an entity without layouting specified");
			layout->updateFinalPosition(update);
		}
	}

	void guiItemStruct::moveToWindow(bool horizontal, bool vertical)
	{
		bool enabled[2] = { horizontal, vertical };
		for (uint32 i = 0; i < 2; i++)
		{
			if (!enabled[i])
				continue;
			real offset = 0;
			if (position[i] + size[i] > impl->outputSize[i])
				offset = (impl->outputSize[i] - size[i]) - position[i];
			else if (position[i] < 0)
				offset = -position[i];
			position[i] += offset;
			contentPosition[i] += offset;
		}
	}

	void guiItemStruct::childrenEmit() const
	{
		guiItemStruct *a = firstChild;
		while (a)
		{
			if (a->widget)
				a->widget->emit();
			else
				a->childrenEmit();
			a = a->nextSibling;
		}
	}

	void guiItemStruct::explicitPosition(vec2 &position, vec2 &size) const
	{
		if (GUI_HAS_COMPONENT(explicitPosition, entity))
		{
			GUI_GET_COMPONENT(explicitPosition, p, entity);
			size = impl->eval<2>(p.size, size);
			position = impl->eval<2>(p.position, position) - p.anchor * size;
		}
		CAGE_ASSERT_RUNTIME(size.valid(), "this item must have explicit size", entity->getName());
	}

	void guiItemStruct::explicitPosition(vec2 &size) const
	{
		vec2 pos = vec2::Nan;
		explicitPosition(pos, size);
		CAGE_ASSERT_RUNTIME(!pos.valid(), "this item may not have explicit position", entity->getName());
		CAGE_ASSERT_RUNTIME(size.valid(), "this item must have explicit size", entity->getName());
	}

	void guiItemStruct::detachParent()
	{
		CAGE_ASSERT_RUNTIME(parent);
		if (prevSibling)
			prevSibling->nextSibling = nextSibling;
		if (nextSibling)
			nextSibling->prevSibling = prevSibling;
		if (parent->firstChild == this)
			parent->firstChild = nextSibling;
		if (parent->lastChild == this)
			parent->lastChild = prevSibling;
		parent = nextSibling = prevSibling = nullptr;
	}

	void guiItemStruct::attachParent(guiItemStruct *newParent)
	{
		if (parent)
			detachParent();
		parent = newParent;
		if (!parent->firstChild)
		{
			parent->firstChild = parent->lastChild = this;
			return;
		}
		prevSibling = parent->lastChild;
		prevSibling->nextSibling = this;
		parent->lastChild = this;
	}

	void positionOffset(vec2 &position, const vec4 &offset)
	{
		position -= vec2(offset);
	}

	void sizeOffset(vec2 &size, const vec4 &offset)
	{
		size += vec2(offset) + vec2(offset[2], offset[3]);
		size = max(size, vec2());
	}
}
