#include "../private.h"

namespace cage
{
	namespace
	{
		struct ListBoxImpl : public WidgetItem
		{
			ListBoxImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy)
			{}

			virtual void initialize() override
			{

			}

			virtual void findRequestedSize() override
			{
				hierarchy->requestedSize = vec2(); // todo this is a temporary hack
			}

			virtual void findFinalPosition(const FinalPosition &update) override
			{

			}

			virtual void emit() const override
			{

			}
		};
	}

	void ListBoxCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<ListBoxImpl>(item);
	}
}
