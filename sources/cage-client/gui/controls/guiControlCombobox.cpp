#include "privateControls.h"

namespace cage
{
	namespace
	{
		struct comboboxItemsListCacheStruct : public controlCacheStruct
		{
			comboboxControlCacheStruct *const combo;

			comboboxItemsListCacheStruct(guiImpl *context, comboboxControlCacheStruct *combo) : controlCacheStruct(context, combo->entity), combo(combo)
			{
				elementType = elementTypeEnum::ComboboxList;
				controlCacheStruct *t = context->cacheRoot->firstChild;
				while (t->nextSibling)
					t = t->nextSibling;
				t->nextSibling = this;
				parent = context->cacheRoot;
				controlMode = cmHidden;
			}

			virtual void updatePositionSize()
			{
				pixelsEnvelopePosition = combo->pixelsEnvelopePosition + combo->pixelsEnvelopeSize * vec2(0, 1);
				pixelsEnvelopeSize = max(pixelsEnvelopeSize, vec2(combo->pixelsEnvelopeSize[0], 0));
				updateContentAndNdcPositionSize();
			}

			virtual void updateRequestedSize()
			{
				controlCacheStruct item(context, 0);
				item.elementType = elementTypeEnum::ComboboxItem;
				item.updateRequestedSize();
				pixelsRequestedSize = item.pixelsRequestedSize;
				pixelsRequestedSize[1] *= combo->itemsCount();
				vec2 dummy;
				context->contentToEnvelope(dummy, pixelsRequestedSize, elementType);
			}

			virtual void graphicPrepare()
			{
				if (controlMode == -1)
					return;
				context->prepareElement(ndcOuterPositionSize, ndcInnerPositionSize, elementType, controlCacheStruct::cmNormal);
				uint32 sel = combo->selected();
				itemCacheStruct *t = combo->firstItem;
				controlCacheStruct item(context, 0);
				item.elementType = elementTypeEnum::ComboboxItem;
				item.updateRequestedSize();
				item.pixelsEnvelopeSize = item.pixelsRequestedSize;
				item.pixelsEnvelopePosition = pixelsContentPosition;
				while (t)
				{
					item.updateContentAndNdcPositionSize();
					controlModeEnum mode = cmNormal;
					if (item.isPointInside(context->mousePosition))
						mode = cmHover;
					else if (sel == 0)
						mode = cmFocus;
					context->prepareElement(item.ndcOuterPositionSize, item.ndcInnerPositionSize, item.elementType, mode);
					context->prepareText(t,
						numeric_cast<sint16>(item.pixelsContentPosition[0]),
						numeric_cast<sint16>(item.pixelsContentPosition[1]),
						numeric_cast<uint16>(item.pixelsContentSize[0]),
						numeric_cast<uint16>(item.pixelsContentSize[1]));
					item.pixelsEnvelopePosition[1] += item.pixelsEnvelopeSize[1];
					t = t->next;
					sel--;
				}
			}

			virtual bool mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
			{
				controlCacheStruct item(context, 0);
				item.elementType = elementTypeEnum::ComboboxItem;
				item.updateRequestedSize();
				item.pixelsEnvelopeSize = item.pixelsRequestedSize;
				item.pixelsEnvelopePosition = pixelsContentPosition;
				uint32 idx = 0;
				itemCacheStruct *t = combo->firstItem;
				while (t)
				{
					item.updateContentAndNdcPositionSize();
					if (item.isPointInside(context->mousePosition))
					{
						combo->selected() = idx;
						context->focusName = 0;
						context->genericEvent.dispatch(entity);
						return true;
					}
					item.pixelsEnvelopePosition[1] += item.pixelsEnvelopeSize[1];
					t = t->next;
					idx++;
				}
				return true;
			}
		};
	}

	comboboxControlCacheStruct::comboboxControlCacheStruct(guiImpl *context, uint32 entity) : controlCacheStruct(context, entity), list(nullptr)
	{
		elementType = elementTypeEnum::ComboboxBase;
		list = context->memoryCache.createObject<comboboxItemsListCacheStruct>(context, this);
	}

	const uint32 comboboxControlCacheStruct::itemsCount() const
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

	uint32 &comboboxControlCacheStruct::selected()
	{
		CAGE_ASSERT_RUNTIME(verifyControlEntity());
		GUI_GET_COMPONENT(control, control, context->entityManager->getEntity(entity));
		return control.ival;
	}

	void comboboxControlCacheStruct::graphicPrepare()
	{
		list->controlMode = context->focusName != entity ? cmHidden : list->isPointInside(context->mousePosition) ? cmHover : cmNormal;

		if (controlMode == -1)
			return;

		context->prepareElement(ndcOuterPositionSize, ndcInnerPositionSize, elementType, controlMode);

		if (!verifyControlEntity())
			return;

		uint32 sel = selected();
		if (sel != -1 && firstItem)
		{
			itemCacheStruct *t = firstItem;
			while (sel-- > 0 && t->next)
				t = t->next;
			context->prepareText(t,
				numeric_cast<sint16>(pixelsContentPosition[0]),
				numeric_cast<sint16>(pixelsContentPosition[1]),
				numeric_cast<uint16>(pixelsContentSize[0]),
				numeric_cast<uint16>(pixelsContentSize[1]));
		}
	}

	bool comboboxControlCacheStruct::mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		if ((buttons & mouseButtonsFlags::Left) == mouseButtonsFlags::Left)
			context->focusName = entity;
		return true;
	}

	bool comboboxControlCacheStruct::keyRepeat(windowClass *win, uint32 key, uint8 scanCode, modifiersFlags modifiers)
	{
		switch (key)
		{
		case 265: // up
		case 263: // left
		{
			uint32 &sel = selected();
			if (sel > 0)
			{
				sel--;
				context->genericEvent.dispatch(entity);
			}
		} break;
		case 262: // right
		case 264: // down
		{
			uint32 &sel = selected();
			if (sel + 1 < itemsCount())
			{
				sel++;
				context->genericEvent.dispatch(entity);
			}
		} break;
		}
		return true;
	}
}