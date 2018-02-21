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
		struct layoutZeroImpl : public layoutBaseStruct
		{
			layoutZeroImpl(guiItemStruct *base) : layoutBaseStruct(base)
			{}

			virtual void initialize() override
			{}

			virtual void updateRequestedSize() override
			{
				guiItemStruct *c = base->firstChild;
				while (c)
				{
					c->updateRequestedSize();
					c = c->nextSibling;
				}
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{
				guiItemStruct *c = base->firstChild;
				while (c)
				{
					updatePositionStruct u(update);
					c->updateFinalPosition(u);
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
