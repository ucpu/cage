#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

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
		struct layoutSplitterImpl : public layoutBaseStruct
		{
			layoutSplitterImpl(guiItemStruct *base) : layoutBaseStruct(base)
			{}

			virtual void initialize() override
			{

			}

			virtual void updateRequestedSize() override
			{

			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{

			}
		};
	}

	void layoutSplitterCreate(guiItemStruct *item)
	{
		item->layout = item->impl->itemsMemory.createObject<layoutSplitterImpl>(item);
	}
}
