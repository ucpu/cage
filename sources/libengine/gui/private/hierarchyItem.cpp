#include <cage-core/config.h>

#include "../private.h"

namespace cage
{
	HierarchyItem::HierarchyItem(GuiImpl *impl, Entity *ent) :
		requestedSize(vec2::Nan()), renderPos(vec2::Nan()), renderSize(vec2::Nan()), clipPos(vec2::Nan()), clipSize(vec2::Nan()),
		impl(impl), ent(ent),
		parent(nullptr), prevSibling(nullptr), nextSibling(nullptr), firstChild(nullptr), lastChild(nullptr),
		item(nullptr), text(nullptr), Image(nullptr),
		order(0), subsidedItem(false)
	{
		CAGE_ASSERT(impl);
	}

	void HierarchyItem::initialize()
	{
		if (item)
			item->initialize();
		if (text)
			text->initialize();
		if (Image)
			Image->initialize();
	}

	void HierarchyItem::findRequestedSize()
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
			HierarchyItem *c = firstChild;
			while (c)
			{
				c->findRequestedSize();
				requestedSize = max(requestedSize, c->requestedSize);
				c = c->nextSibling;
			}
		}
		CAGE_ASSERT(requestedSize.valid());
	}

	void HierarchyItem::findFinalPosition(const FinalPosition &update)
	{
		CAGE_ASSERT(requestedSize.valid());
		CAGE_ASSERT(update.renderPos.valid());
		CAGE_ASSERT(update.renderSize.valid());
		CAGE_ASSERT(update.clipPos.valid());
		CAGE_ASSERT(update.clipSize.valid());

		renderPos = update.renderPos;
		renderSize = update.renderSize;

		FinalPosition u(update);
		clip(u.clipPos, u.clipSize, u.renderPos, u.renderSize); // update the clip rect to intersection with the render rect
		clipPos = u.clipPos;
		clipSize = u.clipSize;

		if (item)
			item->findFinalPosition(u);
		else
		{
			HierarchyItem *c = firstChild;
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

	void HierarchyItem::moveToWindow(bool horizontal, bool vertical)
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

	void HierarchyItem::detachChildren()
	{
		while (firstChild)
			firstChild->detachParent();
	}

	void HierarchyItem::detachParent()
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

	void HierarchyItem::attachParent(HierarchyItem *newParent)
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

	void HierarchyItem::childrenEmit() const
	{
		HierarchyItem *a = firstChild;
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

	void HierarchyItem::generateEventReceivers() const
	{
		if (nextSibling)
			nextSibling->generateEventReceivers();
		if (firstChild)
			firstChild->generateEventReceivers();
		if (item)
			item->generateEventReceivers();
	}
}
