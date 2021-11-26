#include "../private.h"

namespace cage
{
	namespace
	{
		struct ExplicitSizeImpl : public LayoutItem
		{
			const GuiExplicitSizeComponent &data;

			ExplicitSizeImpl(HierarchyItem *hierarchy) : LayoutItem(hierarchy), data(GUI_REF_COMPONENT(ExplicitSize))
			{}

			virtual void initialize() override
			{}

			virtual void findRequestedSize() override
			{
				hierarchy->requestedSize = Vec2();
				for (const auto &c : hierarchy->children)
				{
					c->findRequestedSize();
					hierarchy->requestedSize = max(hierarchy->requestedSize, c->requestedSize);
				}
				for (uint32 i = 0; i < 2; i++)
				{
					if (data.size[i].valid())
						hierarchy->requestedSize[i] = data.size[i];
				}
				CAGE_ASSERT(hierarchy->requestedSize.valid());
			}

			virtual void findFinalPosition(const FinalPosition &update) override
			{
				FinalPosition u(update);
				for (const auto &c : hierarchy->children)
				{
					c->findFinalPosition(u);
				}
			}
		};
	}

	void explicitSizeCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<ExplicitSizeImpl>(item).cast<BaseItem>();
	}
}
