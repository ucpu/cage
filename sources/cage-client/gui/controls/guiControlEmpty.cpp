#include "privateControls.h"

namespace cage
{
	emptyControlCacheStruct::emptyControlCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity)
	{
		elementType = elementTypeEnum::Empty;
	}

	void emptyControlCacheStruct::updateRequestedSize()
	{
		if (firstItem && firstItem->type == itemCacheStruct::itText && firstItem->font)
		{
			fontClass::formatStruct f;
			f.wrapWidth = real::PositiveInfinity;
			firstItem->font->size(firstItem->translatedText, firstItem->translatedLength, f, pixelsRequestedSize);
			vec2 dummy;
			context->contentToEnvelope(dummy, pixelsRequestedSize, elementType);
		}
		else
			pixelsRequestedSize = vec2();

		entityClass *e = context->entityManager->getEntity(entity);
		if (e->hasComponent(context->components.position))
		{
			GUI_GET_COMPONENT(position, p, e);
			pixelsRequestedSize = vec2(
				context->evaluateUnit(p.w, p.wUnit, pixelsRequestedSize[0]),
				context->evaluateUnit(p.h, p.hUnit, pixelsRequestedSize[1])
			);
		}
	}

	void emptyControlCacheStruct::graphicPrepare()
	{
		if (controlMode == -1)
			return;

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

	bool emptyControlCacheStruct::mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point) { return false; }
	bool emptyControlCacheStruct::mouseRelease(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point) { return false; }
	bool emptyControlCacheStruct::mouseMove(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point) { return false; }
	bool emptyControlCacheStruct::mouseWheel(windowClass *win, sint8 wheel, modifiersFlags modifiers, const pointStruct &point) { return false; }
	bool emptyControlCacheStruct::keyPress(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers) { return false; }
	bool emptyControlCacheStruct::keyRepeat(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers) { return false; }
	bool emptyControlCacheStruct::keyRelease(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers) { return false; }
	bool emptyControlCacheStruct::keyChar(windowClass *win, uint32 key) { return false; }
}
