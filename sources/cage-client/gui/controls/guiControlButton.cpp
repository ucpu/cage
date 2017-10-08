#include "privateControls.h"

namespace cage
{
	buttonControlCacheStruct::buttonControlCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity)
	{
		elementType = elementTypeEnum::ButtonNormal;
	}

	void buttonControlCacheStruct::graphicPrepare()
	{
		if (controlMode == -1)
			return;

		bool pressed = controlMode == controlCacheStruct::cmHover && context->focusName == entity && (context->mouseButtons & mouseButtonsFlags::Left) == mouseButtonsFlags::Left;
		context->prepareElement(ndcOuterPositionSize, ndcInnerPositionSize, pressed ? elementTypeEnum::ButtonPressed : elementTypeEnum::ButtonNormal, controlMode);

		if (firstItem)
		{
			CAGE_ASSERT_RUNTIME(!firstItem->next);
			switch (firstItem->type)
			{
			case itemCacheStruct::itText:
				context->prepareText(firstItem,
					numeric_cast<sint16>(pixelsContentPosition[0]),
					numeric_cast<sint16>(pixelsContentPosition[1]),
					numeric_cast<uint16>(pixelsContentSize[0]),
					numeric_cast<uint16>(pixelsContentSize[1]));

				break;
			case itemCacheStruct::itImage:
				context->prepareImage(firstItem, ndcInnerPositionSize);
				break;
			}
		}
	}

	bool buttonControlCacheStruct::mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		if ((buttons & mouseButtonsFlags::Left) == mouseButtonsFlags::Left)
			context->focusName = entity;
		return true;
	}

	bool buttonControlCacheStruct::mouseRelease(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		if (context->focusName == entity && (buttons & mouseButtonsFlags::Left) == mouseButtonsFlags::Left)
			context->genericEvent.dispatch(entity);
		return true;
	}

	bool buttonControlCacheStruct::keyRelease(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers)
	{
		if (key == 257 || key == 32)
			context->genericEvent.dispatch(entity);
		return true;
	}
}