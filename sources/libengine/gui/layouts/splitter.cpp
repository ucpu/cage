#include "../private.h"

namespace cage
{
	namespace
	{
		struct LayoutSplitterImpl : public LayoutItem
		{
			GuiLayoutSplitterComponent &data;

			LayoutSplitterImpl(HierarchyItem *hierarchy) : LayoutItem(hierarchy), data(GUI_REF_COMPONENT(LayoutSplitter))
			{}

			virtual void initialize() override
			{
				uint32 chc = 0;
				HierarchyItem *c = hierarchy->firstChild;
				while (c)
				{
					chc++;
					c = c->nextSibling;
				}
				CAGE_ASSERT(chc == 2, chc, "splitter layout must have exactly two children");
			}

			virtual void findRequestedSize() override
			{
				HierarchyItem *c = hierarchy->firstChild;
				hierarchy->requestedSize = vec2();
				while (c)
				{
					c->findRequestedSize();
					hierarchy->requestedSize[data.vertical] += c->requestedSize[data.vertical];
					hierarchy->requestedSize[!data.vertical] = max(hierarchy->requestedSize[!data.vertical], c->requestedSize[!data.vertical]);
					c = c->nextSibling;
				}
				CAGE_ASSERT(hierarchy->requestedSize.valid());
			}

			virtual void findFinalPosition(const FinalPosition &update) override
			{
				HierarchyItem *a = hierarchy->firstChild, *b = hierarchy->firstChild->nextSibling;
				uint32 axis = data.vertical ? 1 : 0;
				FinalPosition u(update);
				real split = data.inverse ? max(update.renderSize[axis] - b->requestedSize[axis], 0) : a->requestedSize[axis];
				u.renderPos[axis] += 0;
				u.renderSize[axis] = split;
				a->findFinalPosition(u);
				u.renderPos[axis] += split;
				u.renderSize[axis] = max(update.renderSize[axis] - split, 0);
				b->findFinalPosition(u);
			}
		};
	}

	void LayoutSplitterCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<LayoutSplitterImpl>(item);
	}
}
