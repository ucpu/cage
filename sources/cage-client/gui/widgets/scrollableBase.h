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
	struct scrollableBaseStruct : public widgetBaseStruct
	{
		scrollableBaseComponentStruct *scrollable;
		const skinWidgetDefaultsStruct::scrollableBaseStruct *defaults;
		elementTypeEnum elementBase;
		elementTypeEnum elementCaption;

		vec4 frame;

		scrollableBaseStruct(guiItemStruct *base);

		void scrollableInitialize();
		void scrollableUpdateRequestedSize();
		void scrollableUpdateFinalPosition(const updatePositionStruct &update);
		void scrollableEmit();
	};
}
