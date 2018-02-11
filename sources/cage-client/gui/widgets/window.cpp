#include "scrollableBase.h"

namespace cage
{
	namespace
	{
		struct windowImpl : public scrollableBaseStruct
		{
			windowComponent &data;

			windowImpl(guiItemStruct *base) : scrollableBaseStruct(base), data(GUI_REF_COMPONENT(window))
			{
				auto *impl = base->impl;
				GUI_GET_COMPONENT(window, w, base->entity);
				data = w;
				scrollable = &data;
			}

			virtual void initialize() override
			{
				defaults = &skin().defaults.window;
				if ((data.mode & windowModeFlags::Modal) == windowModeFlags::Modal)
					elementBase = elementTypeEnum::WindowBaseModal;
				else
					elementBase = elementTypeEnum::WindowBaseNormal;
				elementCaption = elementTypeEnum::WindowCaption;
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
				// todo emit window buttons and resizer
				base->childrenEmit();
			}
		};
	}

	void windowCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<windowImpl>(item);
	}
}
