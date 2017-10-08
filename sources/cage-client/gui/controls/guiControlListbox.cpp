#include "privateControls.h"

namespace cage
{
	namespace
	{
		const bool getSelection(guiImpl *context, itemCacheStruct *item)
		{
			CAGE_ASSERT_RUNTIME(item->type == itemCacheStruct::itText);
			CAGE_ASSERT_RUNTIME(item->verifyItemEntity(context));
			GUI_GET_COMPONENT(selection, s, context->entityManager->getEntity(item->entity));
			return s.selectionLength > 0;
		}

		void toggleSelection(guiImpl *context, itemCacheStruct *item)
		{
			CAGE_ASSERT_RUNTIME(item->type == itemCacheStruct::itText);
			CAGE_ASSERT_RUNTIME(item->verifyItemEntity(context));
			GUI_GET_COMPONENT(selection, s, context->entityManager->getEntity(item->entity));
			s.selectionLength = s.selectionLength == 0 ? 1 : 0;
			s.selectionStart = 0;
		}
	}

	listboxControlCacheStruct::listboxControlCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity)
	{
		elementType = elementTypeEnum::ListboxList;
	}

	const uint32 listboxControlCacheStruct::itemsCount() const
	{
		uint32 result = 0;
		itemCacheStruct *it = firstItem;
		while (it)
		{
			result++;
			it = it->next;
		}
		return result;
	}

	void listboxControlCacheStruct::updateRequestedSize()
	{
		controlCacheStruct item(context, 0);
		item.elementType = elementTypeEnum::ListboxItem;
		item.updateRequestedSize();
		pixelsRequestedSize = item.pixelsRequestedSize;
		pixelsRequestedSize[1] *= max(itemsCount(), 1u);
		vec2 dummy;
		context->contentToEnvelope(dummy, pixelsRequestedSize, elementType);
	}

	void listboxControlCacheStruct::graphicPrepare()
	{
		if (controlMode == -1)
			return;
		context->prepareElement(ndcOuterPositionSize, ndcInnerPositionSize, elementType, controlMode);
		itemCacheStruct *t = firstItem;
		controlCacheStruct item(context, 0);
		item.elementType = elementTypeEnum::ListboxItem;
		item.updateRequestedSize();
		item.pixelsEnvelopeSize = item.pixelsRequestedSize;
		item.pixelsEnvelopePosition = pixelsContentPosition;
		while (t)
		{
			item.updateContentAndNdcPositionSize();
			controlModeEnum mode = cmNormal;
			if (controlMode == cmDisabled)
				mode = cmDisabled;
			else if (item.isPointInside(context->mousePosition))
				mode = cmHover;
			else if (getSelection(context, t))
				mode = cmFocus;
			context->prepareElement(item.ndcOuterPositionSize, item.ndcInnerPositionSize, item.elementType, mode);
			context->prepareText(t,
				numeric_cast<sint16>(item.pixelsContentPosition[0]),
				numeric_cast<sint16>(item.pixelsContentPosition[1]),
				numeric_cast<uint16>(item.pixelsContentSize[0]),
				numeric_cast<uint16>(item.pixelsContentSize[1]));
			item.pixelsEnvelopePosition[1] += item.pixelsEnvelopeSize[1];
			t = t->next;
		}
	}

	bool listboxControlCacheStruct::mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		controlCacheStruct item(context, 0);
		item.elementType = elementTypeEnum::ComboboxItem;
		item.updateRequestedSize();
		item.pixelsEnvelopeSize = item.pixelsRequestedSize;
		item.pixelsEnvelopePosition = pixelsContentPosition;
		itemCacheStruct *t = firstItem;
		while (t)
		{
			item.updateContentAndNdcPositionSize();
			if (item.isPointInside(context->mousePosition))
			{
				toggleSelection(context, t);
				context->genericEvent.dispatch(t->entity);
				context->genericEvent.dispatch(entity);
				return true;
			}
			item.pixelsEnvelopePosition[1] += item.pixelsEnvelopeSize[1];
			t = t->next;
		}
		return true;
	}

	bool listboxControlCacheStruct::keyRepeat(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers)
	{
		return true;
	}
}