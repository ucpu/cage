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
		struct layoutRadialImpl : public layoutBaseStruct
		{
			layoutRadialImpl(guiItemStruct *base) : layoutBaseStruct(base)
			{}

			virtual void initialize() override
			{
			
			}

			virtual void findRequestedSize() override
			{

			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{

			}
		};
	}

	void layoutRadialCreate(guiItemStruct *item)
	{
		item->layout = item->impl->itemsMemory.createObject<layoutRadialImpl>(item);
	}
}
