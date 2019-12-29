#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include "../private.h"

namespace cage
{
	hierarchyItemStruct::hierarchyItemStruct(guiImpl *impl, Entity *ent) :
		requestedSize(vec2::Nan()), renderPos(vec2::Nan()), renderSize(vec2::Nan()), clipPos(vec2::Nan()), clipSize(vec2::Nan()),
		impl(impl), ent(ent),
		parent(nullptr), prevSibling(nullptr), nextSibling(nullptr), firstChild(nullptr), lastChild(nullptr),
		item(nullptr), text(nullptr), Image(nullptr),
		order(0), subsidedItem(false)
	{
		CAGE_ASSERT(impl);
	}

	void hierarchyItemStruct::initialize()
	{
		if (item)
			item->initialize();
		if (text)
			text->initialize();
		if (Image)
			Image->initialize();
	}

	void hierarchyItemStruct::findRequestedSize()
	{
		if (item)
			item->findRequestedSize();
		else if (text)
			requestedSize = text->findRequestedSize();
		else if (Image)
			requestedSize = Image->findRequestedSize();
		else
		{
			requestedSize = vec2();
			hierarchyItemStruct *c = firstChild;
			while (c)
			{
				c->findRequestedSize();
				requestedSize = max(requestedSize, c->requestedSize);
				c = c->nextSibling;
			}
		}
		CAGE_ASSERT(requestedSize.valid());
	}

	void hierarchyItemStruct::findFinalPosition(const finalPositionStruct &update)
	{
		CAGE_ASSERT(requestedSize.valid());
		CAGE_ASSERT(update.renderPos.valid());
		CAGE_ASSERT(update.renderSize.valid());
		CAGE_ASSERT(update.clipPos.valid());
		CAGE_ASSERT(update.clipSize.valid());

		renderPos = update.renderPos;
		renderSize = update.renderSize;

		finalPositionStruct u(update);
		clip(u.clipPos, u.clipSize, u.renderPos, u.renderSize); // update the clip rect to intersection with the render rect
		clipPos = u.clipPos;
		clipSize = u.clipSize;

		if (item)
			item->findFinalPosition(u);
		else
		{
			hierarchyItemStruct *c = firstChild;
			while (c)
			{
				c->findFinalPosition(u);
				c = c->nextSibling;
			}
		}

		CAGE_ASSERT(renderPos.valid());
		CAGE_ASSERT(renderSize.valid());
		CAGE_ASSERT(clipPos.valid());
		CAGE_ASSERT(clipSize.valid());
		for (uint32 a = 0; a < 2; a++)
		{
			CAGE_ASSERT(clipPos[a] >= u.clipPos[a], clipPos, u.clipPos);
			CAGE_ASSERT(clipPos[a] + clipSize[a] <= u.clipPos[a] + u.clipSize[a], clipPos, clipSize, u.clipPos, u.clipSize);
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
		CAGE_ASSERT(parent);
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

	void hierarchyItemStruct::generateEventReceivers() const
	{
		if (nextSibling)
			nextSibling->generateEventReceivers();
		if (firstChild)
			firstChild->generateEventReceivers();
		if (item)
			item->generateEventReceivers();
	}
}
