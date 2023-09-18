#include "../private.h"

namespace cage
{
	namespace
	{
		struct SplitImpl : public LayoutItem
		{
			const GuiLayoutSplitComponent &data;

			SplitImpl(HierarchyItem *hierarchy) : LayoutItem(hierarchy), data(GUI_REF_COMPONENT(LayoutSplit)) {}

			void initialize() override
			{
				CAGE_ASSERT(!hierarchy->text);
				CAGE_ASSERT(!hierarchy->image);
			}

			void findRequestedSize() override
			{
				hierarchy->requestedSize = Vec2();
				for (const auto &c : hierarchy->children)
				{
					c->findRequestedSize();
					hierarchy->requestedSize = max(hierarchy->requestedSize, c->requestedSize);
				}
				hierarchy->requestedSize[data.vertical] *= hierarchy->children.size();
				CAGE_ASSERT(hierarchy->requestedSize.valid());
			}

			void findFinalPosition(const FinalPosition &update) override
			{
				if (hierarchy->children.empty())
					return;
				const uint32 h = data.vertical ? 1 : hierarchy->children.size();
				const uint32 v = data.vertical ? hierarchy->children.size() : 1;
				const Vec2 s = update.renderSize / Vec2(h, v);
				CAGE_ASSERT(s.valid());
				Real offset = 0;
				for (const auto &c : hierarchy->children)
				{
					FinalPosition u(update);
					u.renderSize = s;
					u.renderPos[data.vertical] += offset;
					if (data.crossAlign.valid())
					{
						u.renderSize[!data.vertical] = min(c->requestedSize[!data.vertical], u.renderSize[!data.vertical]);
						u.renderPos[!data.vertical] += (update.renderSize[!data.vertical] - u.renderSize[!data.vertical]) * data.crossAlign;
					}
					c->findFinalPosition(u);
					offset += s[data.vertical];
				}
			}
		};
	}

	void LayoutSplitCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<SplitImpl>(item).cast<BaseItem>();
	}
}
