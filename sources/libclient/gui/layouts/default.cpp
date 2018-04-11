#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
#include "../private.h"

namespace cage
{
	namespace
	{
		struct layoutDefaultImpl : public layoutBaseStruct
		{
			layoutDefaultImpl(guiItemStruct *base) : layoutBaseStruct(base)
			{}

			virtual void initialize() override
			{}

			virtual void updateRequestedSize() override
			{
				base->requestedSize = vec2();
				guiItemStruct *c = base->firstChild;
				while (c)
				{
					c->updateRequestedSize();
					vec2 pos;
					c->explicitPosition(pos, c->requestedSize);
					base->requestedSize = max(base->requestedSize, pos + c->requestedSize);
					c = c->nextSibling;
				}
				CAGE_ASSERT_RUNTIME(base->requestedSize.valid());
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{
				guiItemStruct *c = base->firstChild;
				while (c)
				{
					vec2 pos, size = c->requestedSize;
					c->explicitPosition(pos, size);
					updatePositionStruct u(update);
					u.position += pos;
					u.size = size;
					c->updateFinalPosition(u);
					c = c->nextSibling;
				}
			}
		};
	}

	void layoutDefaultCreate(guiItemStruct *item)
	{
		item->layout = item->impl->itemsMemory.createObject<layoutDefaultImpl>(item);
	}
}
