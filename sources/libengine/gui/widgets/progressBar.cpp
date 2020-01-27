#include "../private.h"

namespace cage
{
	namespace
	{
		struct ProgressBarImpl : public WidgetItem
		{
			ProgressBarImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy)
			{}

			virtual void initialize() override
			{

			}

			virtual void findRequestedSize() override
			{
				hierarchy->requestedSize = vec2(); // todo this is a temporary hack
			}

			virtual void emit() const override
			{

			}
		};
	}

	void ProgressBarCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<ProgressBarImpl>(item);
	}
}
