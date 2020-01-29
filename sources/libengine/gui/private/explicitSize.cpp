#include "../private.h"

namespace cage
{
	namespace
	{
		struct ExplicitSizeImpl : public LayoutItem
		{
			GuiExplicitSizeComponent &data;

			ExplicitSizeImpl(HierarchyItem *hierarchy) : LayoutItem(hierarchy), data(GUI_REF_COMPONENT(ExplicitSize))
			{}

			virtual void initialize() override
			{}

			virtual void findRequestedSize() override
			{
				HierarchyItem *c = hierarchy->firstChild;
				hierarchy->requestedSize = vec2();
				while (c)
				{
					c->findRequestedSize();
					hierarchy->requestedSize = max(hierarchy->requestedSize, c->requestedSize);
					c = c->nextSibling;
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
				HierarchyItem *c = hierarchy->firstChild;
				while (c)
				{
					c->findFinalPosition(u);
					c = c->nextSibling;
				}
			}
		};
	}

	void explicitSizeCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<ExplicitSizeImpl>(item);
	}
}
