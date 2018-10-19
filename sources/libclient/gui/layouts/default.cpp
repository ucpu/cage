#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

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
		struct layoutDefaultImpl : public layoutItemStruct
		{
			layoutDefaultImpl(hierarchyItemStruct *hierarchy) : layoutItemStruct(hierarchy)
			{}

			virtual void initialize() override
			{}

			virtual void findRequestedSize() override
			{
				hierarchy->requestedSize = vec2();
				hierarchyItemStruct *c = hierarchy->firstChild;
				while (c)
				{
					c->findRequestedSize();
					hierarchy->requestedSize = max(hierarchy->requestedSize, c->requestedSize);
					c = c->nextSibling;
				}
			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{
				hierarchyItemStruct *c = hierarchy->firstChild;
				while (c)
				{
					finalPositionStruct u(update);
					c->findFinalPosition(u);
					c = c->nextSibling;
				}
			}
		};
	}

	void layoutDefaultCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT_RUNTIME(!item->item);
		item->item = item->impl->itemsMemory.createObject<layoutDefaultImpl>(item);
	}
}
