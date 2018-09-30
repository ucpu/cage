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
		struct layoutZeroImpl : public layoutBaseStruct
		{
			layoutZeroImpl(guiItemStruct *base) : layoutBaseStruct(base)
			{}

			virtual void initialize() override
			{}

			virtual void findRequestedSize() override
			{
				guiItemStruct *c = base->firstChild;
				while (c)
				{
					c->findRequestedSize();
					c = c->nextSibling;
				}
				base->requestedSize = vec2();
			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{
				guiItemStruct *c = base->firstChild;
				while (c)
				{
					finalPositionStruct u(update);
					c->findFinalPosition(u);
					c = c->nextSibling;
				}
			}
		};
	}

	void layoutZeroCreate(guiItemStruct *item)
	{
		item->layout = item->impl->itemsMemory.createObject<layoutZeroImpl>(item);
	}
}
