#include "privateControls.h"

namespace cage
{
	textareaControlCacheStruct::textareaControlCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity)
	{
		elementType = elementTypeEnum::Textarea;
	}
}