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
		struct layoutDefaultImpl : public layoutBaseStruct
		{
			layoutDefaultImpl(guiItemStruct *base) : layoutBaseStruct(base)
			{}

			virtual void initialize() override
			{}

			virtual void findRequestedSize() override
			{
				base->requestedSize = vec2();
				guiItemStruct *c = base->firstChild;
				while (c)
				{
					c->findRequestedSize();
					vec2 pos;
					c->checkExplicitPosition(pos, c->requestedSize);
					base->requestedSize = max(base->requestedSize, pos + c->requestedSize);
					c = c->nextSibling;
				}
			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{
				guiItemStruct *c = base->firstChild;
				while (c)
				{
					vec2 pos, size = c->requestedSize;
					c->checkExplicitPosition(pos, size);
					finalPositionStruct u(update);
					u.renderPos += pos;
					u.renderSize = size;
					c->findFinalPosition(u);
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
