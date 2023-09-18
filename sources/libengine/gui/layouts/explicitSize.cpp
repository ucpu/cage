#include "../private.h"

namespace cage
{
	namespace
	{
		struct ExplicitSizeImpl : public LayoutItem
		{
			const GuiExplicitSizeComponent &data;

			ExplicitSizeImpl(HierarchyItem *hierarchy) : LayoutItem(hierarchy), data(GUI_REF_COMPONENT(ExplicitSize)) {}

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.size() == 1);
				CAGE_ASSERT(!hierarchy->text);
				CAGE_ASSERT(!hierarchy->image);
			}

			void findRequestedSize() override
			{
				hierarchy->children[0]->findRequestedSize();
				hierarchy->requestedSize = hierarchy->children[0]->requestedSize;
				for (uint32 i = 0; i < 2; i++)
				{
					if (data.size[i].valid())
						hierarchy->requestedSize[i] = data.size[i];
				}
				CAGE_ASSERT(hierarchy->requestedSize.valid());
			}

			void findFinalPosition(const FinalPosition &update) override { hierarchy->children[0]->findFinalPosition(update); }
		};
	}

	void explicitSizeCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<ExplicitSizeImpl>(item).cast<BaseItem>();
	}
}
