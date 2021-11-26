#include "../private.h"

namespace cage
{
	namespace
	{
		struct LayoutSplitterImpl : public LayoutItem
		{
			const GuiLayoutSplitterComponent &data;

			LayoutSplitterImpl(HierarchyItem *hierarchy) : LayoutItem(hierarchy), data(GUI_REF_COMPONENT(LayoutSplitter))
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.size() == 2);
			}

			virtual void findRequestedSize() override
			{
				hierarchy->requestedSize = Vec2();
				for (const auto &c : hierarchy->children)
				{
					c->findRequestedSize();
					hierarchy->requestedSize[data.vertical] += c->requestedSize[data.vertical];
					hierarchy->requestedSize[!data.vertical] = max(hierarchy->requestedSize[!data.vertical], c->requestedSize[!data.vertical]);
				}
				CAGE_ASSERT(hierarchy->requestedSize.valid());
			}

			virtual void findFinalPosition(const FinalPosition &update) override
			{
				HierarchyItem *a = +hierarchy->children[0], *b = +hierarchy->children[1];
				const uint32 axis = data.vertical ? 1 : 0;
				const Real split = data.inverse ? max(update.renderSize[axis] - b->requestedSize[axis], 0) : a->requestedSize[axis];
				FinalPosition u(update);
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
		item->item = item->impl->memory->createHolder<LayoutSplitterImpl>(item).cast<BaseItem>();
	}
}
