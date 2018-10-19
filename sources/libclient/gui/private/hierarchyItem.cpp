#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "../private.h"

namespace cage
{
	hierarchyItemStruct::hierarchyItemStruct(guiImpl *impl, entityClass *entity) :
		requestedSize(vec2::Nan), renderPos(vec2::Nan), renderSize(vec2::Nan), clipPos(vec2::Nan), clipSize(vec2::Nan),
		impl(impl), entity(entity),
		parent(nullptr), prevSibling(nullptr), nextSibling(nullptr), firstChild(nullptr), lastChild(nullptr),
		item(nullptr), text(nullptr), image(nullptr),
		order(0), subsidedItem(false)
	{
		CAGE_ASSERT_RUNTIME(impl);
	}

	void hierarchyItemStruct::initialize()
	{
		if (item)
			item->initialize();
		if (text)
			text->initialize();
		if (image)
			image->initialize();
	}

	void hierarchyItemStruct::findRequestedSize()
	{
		if (item)
			item->findRequestedSize();
		else if (text)
			requestedSize = text->findRequestedSize();
		else if (image)
			requestedSize = image->findRequestedSize();
		CAGE_ASSERT_RUNTIME(requestedSize.valid());
	}

	void hierarchyItemStruct::findFinalPosition(const finalPositionStruct &update)
	{
		{
			uint32 name = entity ? entity->name() : 0;
			CAGE_ASSERT_RUNTIME(item, "trying to layout an entity without layouting specified", name);
		}

		CAGE_ASSERT_RUNTIME(requestedSize.valid());
		CAGE_ASSERT_RUNTIME(update.renderPos.valid());
		CAGE_ASSERT_RUNTIME(update.renderSize.valid());
		CAGE_ASSERT_RUNTIME(update.clipPos.valid());
		CAGE_ASSERT_RUNTIME(update.clipSize.valid());

		renderPos = update.renderPos;
		renderSize = update.renderSize;

		finalPositionStruct u(update);
		u.clip(u.renderPos, u.renderSize);
		clipPos = u.clipPos;
		clipSize = u.clipSize;

		item->findFinalPosition(u);

		CAGE_ASSERT_RUNTIME(renderPos.valid());
		CAGE_ASSERT_RUNTIME(renderSize.valid());
		CAGE_ASSERT_RUNTIME(clipPos.valid());
		CAGE_ASSERT_RUNTIME(clipSize.valid());
		for (uint32 a = 0; a < 2; a++)
		{
			CAGE_ASSERT_RUNTIME(clipPos[a] >= u.clipPos[a], clipPos, u.clipPos);
			CAGE_ASSERT_RUNTIME(clipPos[a] + clipSize[a] <= u.clipPos[a] + u.clipSize[a], clipPos, clipSize, u.clipPos, u.clipSize);
		}
	}

	void hierarchyItemStruct::moveToWindow(bool horizontal, bool vertical)
	{
		bool enabled[2] = { horizontal, vertical };
		for (uint32 i = 0; i < 2; i++)
		{
			if (!enabled[i])
				continue;
			real offset = 0;
			if (renderPos[i] + renderSize[i] > impl->outputSize[i])
				offset = (impl->outputSize[i] - renderSize[i]) - renderPos[i];
			else if (renderPos[i] < 0)
				offset = -renderPos[i];
			renderPos[i] += offset;
		}
	}

	void hierarchyItemStruct::detachChildren()
	{
		while (firstChild)
			firstChild->detachParent();
	}

	void hierarchyItemStruct::detachParent()
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

	void hierarchyItemStruct::attachParent(hierarchyItemStruct *newParent)
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

	void hierarchyItemStruct::childrenEmit() const
	{
		hierarchyItemStruct *a = firstChild;
		while (a)
		{
			if (a->item)
				a->item->emit();
			else
				a->childrenEmit();
			a->emitDebug();
			a = a->nextSibling;
		}
	}
}
