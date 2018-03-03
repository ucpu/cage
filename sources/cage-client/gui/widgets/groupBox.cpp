#include "scrollableBase.h"

namespace cage
{
	namespace
	{
		struct groupBoxImpl : public scrollableBaseStruct
		{
			groupBoxComponent &data;

			groupBoxImpl(guiItemStruct *base) : scrollableBaseStruct(base), data(GUI_REF_COMPONENT(groupBox))
			{
				auto *impl = base->impl;
				GUI_GET_COMPONENT(groupBox, gb, base->entity);
				data = gb;
				scrollable = &data;
			}

			virtual void initialize() override
			{
				defaults = &skin().defaults.groupBox;
				switch (data.type)
				{
				case groupBoxTypeEnum::Invisible:
					CAGE_ASSERT_RUNTIME(!base->text, "invisible group box may not have caption");
					break;
				case groupBoxTypeEnum::Cell:
					elementBase = elementTypeEnum::GroupCell;
					CAGE_ASSERT_RUNTIME(!base->text, "cell group box may not have caption");
					break;
				case groupBoxTypeEnum::Panel:
				case groupBoxTypeEnum::Spoiler:
					elementBase = elementTypeEnum::GroupPanel;
					break;
				}
				if (data.type == groupBoxTypeEnum::Spoiler || base->text)
					elementCaption = elementTypeEnum::GroupCaption;
				if (data.type == groupBoxTypeEnum::Spoiler && data.spoilerCollapsed)
					base->detachChildren();
				scrollableInitialize();
			}

			virtual void updateRequestedSize() override
			{
				scrollableUpdateRequestedSize();
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{
				scrollableUpdateFinalPosition(update);
			}

			virtual void emit() const override
			{
				scrollableEmit();
				// todo emit spoiler icon
			}

			void collapse(guiItemStruct *item)
			{
				if (!item->widget)
					return;
				groupBoxImpl *b = dynamic_cast<groupBoxImpl*>(item->widget);
				if (!b)
					return;
				if (b->data.type == groupBoxTypeEnum::Spoiler)
					b->data.spoilerCollapsed = true;
			}

			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				makeFocused();
				if (buttons != mouseButtonsFlags::Left)
					return true;
				if (modifiers != modifiersFlags::None)
					return true;
				if (data.type == groupBoxTypeEnum::Spoiler)
				{
					vec2 p = base->position;
					vec2 s = vec2(base->size[0], defaults->captionHeight);
					offset(p, s, -defaults->baseMargin * vec4(1, 1, 1, 0));
					if (pointInside(p, s, point))
					{
						data.spoilerCollapsed = !data.spoilerCollapsed;
						guiItemStruct *i = base->prevSibling;
						while (i)
						{
							collapse(i);
							i = i->prevSibling;
						}
						i = base->nextSibling;
						while (i)
						{
							collapse(i);
							i = i->nextSibling;
						}
					}
				}
				return true;
			}
		};
	}

	void groupBoxCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<groupBoxImpl>(item);
	}
}
