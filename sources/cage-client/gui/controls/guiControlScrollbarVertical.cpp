#include "privateControls.h"

namespace cage
{
	scrollbarVerticalControlCacheStruct::scrollbarVerticalControlCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity)
	{
		elementType = elementTypeEnum::ScrollbarVerticalPanel;
	}
}