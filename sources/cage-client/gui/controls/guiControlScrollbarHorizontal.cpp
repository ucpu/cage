#include "privateControls.h"

namespace cage
{
	scrollbarHorizontalControlCacheStruct::scrollbarHorizontalControlCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity)
	{
		elementType = elementTypeEnum::ScrollbarHorizontalPanel;
	}
}