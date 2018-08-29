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
	struct scrollableBaseStruct : public widgetBaseStruct
	{
		scrollableBaseComponentStruct *scrollable;
		const skinWidgetDefaultsStruct::scrollableBaseStruct *defaults;
		elementTypeEnum elementBase;
		elementTypeEnum elementCaption;

		scrollableBaseStruct(guiItemStruct *base);

		void scrollableInitialize();
		void scrollableUpdateRequestedSize();
		void scrollableUpdateFinalPosition(const finalPositionStruct &update);
		void scrollableEmit() const;
	};
}
