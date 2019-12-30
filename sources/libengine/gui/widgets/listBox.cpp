#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
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
