#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "../private.h"

namespace cage
{
	namespace
	{
		struct progressBarImpl : public widgetItemStruct
		{
			progressBarImpl(hierarchyItemStruct *hierarchy) : widgetItemStruct(hierarchy)
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

	void progressBarCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT_RUNTIME(!item->item);
		item->item = item->impl->itemsMemory.createObject<progressBarImpl>(item);
	}
}
