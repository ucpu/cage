#include "../private.h"

namespace cage
{
	namespace
	{
		struct AlignmentImpl : public LayoutItem
		{
			const GuiLayoutAlignmentComponent &data;

			AlignmentImpl(HierarchyItem *hierarchy) : LayoutItem(hierarchy), data(GUI_REF_COMPONENT(LayoutAlignment)) { ensureItemHasLayout(hierarchy); }

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.size() == 1);
				CAGE_ASSERT(!hierarchy->text);
				CAGE_ASSERT(!hierarchy->image);
				for (uint32 i = 0; i < 2; i++)
				{
					CAGE_ASSERT(!data.alignment[i].valid() || data.alignment[i] == saturate(data.alignment[i]));
				}
			}

			void findRequestedSize() override
			{
				hierarchy->children[0]->findRequestedSize();
				hierarchy->requestedSize = hierarchy->children[0]->requestedSize;
			}

			void findFinalPosition(const FinalPosition &update) override
			{
				FinalPosition u(update);
				for (uint32 a = 0; a < 2; a++)
				{
					if (data.alignment[a].valid())
					{
						u.renderSize[a] = min(hierarchy->requestedSize[a], u.renderSize[a]);
						u.renderPos[a] += (update.renderSize[a] - u.renderSize[a]) * data.alignment[a];
					}
				}
				hierarchy->children[0]->findFinalPosition(u);
			}
		};
	}

	void alignmentCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<AlignmentImpl>(item).cast<BaseItem>();
	}
}
