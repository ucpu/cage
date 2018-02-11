#include "scrollableBase.h"

namespace cage
{
	void layoutDefaultCreate(guiItemStruct *item);

	scrollableBaseStruct::scrollableBaseStruct(guiItemStruct * base) : widgetBaseStruct(base), scrollable(nullptr), defaults(nullptr), elementBase(elementTypeEnum::InvalidElement), elementCaption(elementTypeEnum::InvalidElement)
	{}

	void scrollableBaseStruct::scrollableInitialize()
	{
		CAGE_ASSERT_RUNTIME(scrollable);
		CAGE_ASSERT_RUNTIME(defaults);
		if (!base->layout)
			layoutDefaultCreate(base);
	}

	void scrollableBaseStruct::scrollableUpdateRequestedSize()
	{
		base->layout->updateRequestedSize();
		frame = vec4();
		if (elementBase != elementTypeEnum::InvalidElement)
		{
			frame += skin().layouts[(uint32)elementBase].border;
		}
		if (elementCaption != elementTypeEnum::InvalidElement)
		{
			vec4 b = skin().layouts[(uint32)elementCaption].border;
			frame[1] += b[1] + b[3] + defaults->captionHeight;
		}
		frame += defaults->baseMargin;
		frame += defaults->contentPadding;
		// todo scrollbars
		sizeOffset(base->requestedSize, frame);
	}

	void scrollableBaseStruct::scrollableUpdateFinalPosition(const updatePositionStruct &update)
	{
		base->contentPosition = update.position;
		base->contentSize = update.size;
		positionOffset(base->contentPosition, -frame);
		sizeOffset(base->contentSize, -frame);
		updatePositionStruct u(update);
		u.position = base->contentPosition;
		u.size = base->contentSize;
		positionOffset(u.position, -frame);
		sizeOffset(u.size, -frame);
		base->layout->updateFinalPosition(u);
	}

	void scrollableBaseStruct::scrollableEmit()
	{

	}
}
