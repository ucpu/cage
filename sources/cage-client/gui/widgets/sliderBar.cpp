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
		struct sliderBarImpl : public widgetBaseStruct
		{
			sliderBarImpl(guiItemStruct *base) : widgetBaseStruct(base)
			{}

			virtual void initialize() override
			{

			}

			virtual void updateRequestedSize() override
			{
				base->requestedSize = vec2(150, 30); // todo this is a temporary hack
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{

			}

			virtual void emit() const override
			{
				emitElement(elementTypeEnum::SliderHorizontalPanel, 0, vec4()); // todo this is a temporary hack
			}
		};
	}

	void sliderBarCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<sliderBarImpl>(item);
	}
}
