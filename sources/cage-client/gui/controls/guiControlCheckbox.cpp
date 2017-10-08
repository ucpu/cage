#include "privateControls.h"

namespace cage
{
	checkboxControlCacheStruct::checkboxControlCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity)
	{
		GUI_GET_COMPONENT(control, c, context->entityManager->getEntity(entity));
		switch (c.ival)
		{
		case 0: elementType = elementTypeEnum::CheckboxUnchecked; break;
		case 1: elementType = elementTypeEnum::CheckboxChecked; break;
		case 2: elementType = elementTypeEnum::CheckboxUndetermined; break;
		default: CAGE_THROW_CRITICAL(exception, "invalid checkbox ival");
		}
	}

	void checkboxControlCacheStruct::click()
	{
		CAGE_ASSERT_RUNTIME(verifyControlEntity());
		GUI_GET_COMPONENT(control, c, context->entityManager->getEntity(entity));
		c.ival = c.ival == 0 ? 1 : 0;
		context->genericEvent.dispatch(entity);
	}

	void checkboxControlCacheStruct::graphicPrepare()
	{
		if (controlMode == -1)
			return;
		context->prepareElement(ndcOuterPositionSize, ndcInnerPositionSize, elementType, controlMode);
	}

	bool checkboxControlCacheStruct::mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		if ((buttons & mouseButtonsFlags::Left) == mouseButtonsFlags::Left)
			context->focusName = entity;
		return true;
	}

	bool checkboxControlCacheStruct::mouseRelease(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		if (context->focusName == entity && (buttons & mouseButtonsFlags::Left) == mouseButtonsFlags::Left)
			click();
		return true;
	}

	bool checkboxControlCacheStruct::keyRelease(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers)
	{
		if (key == 257 || key == 32)
			click();
		return true;
	}
}