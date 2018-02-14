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
		struct taskBarImpl : public widgetBaseStruct
		{
			taskBarImpl(guiItemStruct *base) : widgetBaseStruct(base)
			{}

			virtual void initialize() override
			{

			}

			virtual void updateRequestedSize() override
			{
				base->requestedSize = vec2(); // todo this is a temporary hack
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{

			}

			virtual void emit() const override
			{

			}
		};
	}

	void taskBarCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<taskBarImpl>(item);
	}
}
