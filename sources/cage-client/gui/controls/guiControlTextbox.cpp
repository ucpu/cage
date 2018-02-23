#include <cage-core/core.h>
#include <cage-core/utility/utf.h>
#include "privateControls.h"

namespace cage
{
	textboxControlCacheStruct::textboxControlCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity)
	{
		elementType = elementTypeEnum::Textbox;
	}

	void textboxControlCacheStruct::graphicPrepare()
	{
		CAGE_ASSERT_RUNTIME(firstItem && firstItem->type == itemCacheStruct::itText && !firstItem->next);

		if (controlMode == -1)
			return;

		context->prepareElement(ndcOuterPositionSize, ndcInnerPositionSize, elementType, controlMode);

		uint32 cursor = -1;
		if (context->focusName == entity && verifyControlEntity() && firstItem->verifyItemEntity(context))
		{
			GUI_GET_COMPONENT(text, txt, context->entityManager->getEntity(firstItem->entity));
			uint32 len = countCharacters(txt.value);
			GUI_GET_COMPONENT(control, control, context->entityManager->getEntity(entity));
			if (control.ival > len)
				control.ival = len;
			cursor = control.ival;
		}

		context->prepareText(firstItem,
			numeric_cast<sint16>(pixelsContentPosition[0]),
			numeric_cast<sint16>(pixelsContentPosition[1]),
			numeric_cast<uint16>(pixelsContentSize[0]),
			numeric_cast<uint16>(pixelsContentSize[1]),
			cursor);
	}

	bool textboxControlCacheStruct::mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		if (!firstItem->verifyItemEntity(context))
			return true;

		if ((buttons & mouseButtonsFlags::Left) == mouseButtonsFlags::Left)
		{
			context->focusName = entity;
			fontClass::formatStruct f;
			f.align = firstItem->align;
			f.lineSpacing = firstItem->lineSpacing;
			f.wrapWidth = numeric_cast<uint16>(pixelsContentSize[0]);
			GUI_GET_COMPONENT(control, control, context->entityManager->getEntity(entity));
			vec2 dummy;
			firstItem->font->size(firstItem->translatedText, firstItem->translatedLength, f, dummy, context->mousePosition - pixelsContentPosition, control.ival);
		}
		return true;
	}

	bool textboxControlCacheStruct::keyRepeat(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers)
	{
		if (!firstItem->verifyItemEntity(context))
			return true;
		GUI_GET_COMPONENT(text, txt, context->entityManager->getEntity(firstItem->entity));
		uint32 buffer[string::MaxLength];
		uint32 len = string::MaxLength - 1;
		convert8to32(buffer, len, txt.value);
		GUI_GET_COMPONENT(control, control, context->entityManager->getEntity(entity));
		if (control.ival > len)
			control.ival = len;
		switch (key)
		{
		case 263: // left
		{
			if (control.ival > 0)
				control.ival--;
		} break;
		case 262: // right
		{
			if (control.ival < len)
				control.ival++;
		} break;
		case 259: // backspace
		{
			if (len == 0 || control.ival == 0)
				return true;
			detail::memmove(buffer + (control.ival - 1), buffer + control.ival, (len - control.ival) * 4);
			control.ival--;
			len--;
			txt.value = convert32to8(buffer, len);
			context->genericEvent.dispatch(entity);
		} break;
		case 261: // delete
		{
			if (control.ival == len)
				return true;
			detail::memmove(buffer + control.ival, buffer + (control.ival + 1), (len - control.ival - 1) * 4);
			len--;
			txt.value = convert32to8(buffer, len);
			context->genericEvent.dispatch(entity);
		} break;
		}
		return true;
	}

	bool textboxControlCacheStruct::keyChar(windowClass *win, uint32 key)
	{
		if (!firstItem->verifyItemEntity(context))
			return true;
		GUI_GET_COMPONENT(text, txt, context->entityManager->getEntity(firstItem->entity));
		uint32 buffer[string::MaxLength];
		uint32 len = string::MaxLength - 1;
		convert8to32(buffer, len, txt.value);
		GUI_GET_COMPONENT(control, control, context->entityManager->getEntity(entity));
		if (control.ival > len)
			control.ival = len;
		if (len >= string::MaxLength - 1)
			return true;
		detail::memmove(buffer + (control.ival + 1), buffer + control.ival, (len - control.ival) * 4);
		buffer[control.ival] = key;
		control.ival++;
		len++;
		txt.value = convert32to8(buffer, len);
		context->genericEvent.dispatch(entity);
		return true;
	}
}
