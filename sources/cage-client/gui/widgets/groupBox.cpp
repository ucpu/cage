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
				case groupBoxTypeEnum::Cell:
					elementBase = elementTypeEnum::GroupCell;
					break;
				case groupBoxTypeEnum::Panel:
				case groupBoxTypeEnum::Spoiler:
					elementBase = elementTypeEnum::GroupPanel;
					break;
				}
				if (data.type == groupBoxTypeEnum::Spoiler || base->text)
					elementCaption = elementTypeEnum::GroupCaption;
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

			virtual void emit() override
			{
				scrollableEmit();
				// todo emit spoiler icon
				base->childrenEmit();
			}
		};
	}

	void groupBoxCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<groupBoxImpl>(item);
	}
}
