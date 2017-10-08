#include "privateControls.h"

namespace cage
{
	radioButtonControlCacheStruct::radioButtonControlCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity)
	{
		elementType = elementTypeEnum::RadioUnchecked;
	}

	radioGroupControlCacheStruct::radioGroupControlCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity)
	{
		elementType = elementTypeEnum::Empty;
	}

	void radioGroupControlCacheStruct::updatePositionSize()
	{
		updateContentAndNdcPositionSize();
	}
}