#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
#include "../private.h"

namespace cage
{
	namespace
	{
		struct buttonImpl : public widgetBaseStruct
		{
			buttonImpl(guiItemStruct *base) : widgetBaseStruct(base)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(!base->firstChild, "input box may not have children");
				CAGE_ASSERT_RUNTIME(!base->layout, "input box may not have layout");
				if (base->text)
					base->text->text.apply(skin().defaults.button.textFormat, base->impl);
			}

			virtual void updateRequestedSize() override
			{
				base->requestedSize = skin().defaults.button.size;
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{
				base->updateContentPosition(skin().defaults.button.margin);
			}

			virtual void emit() const override
			{
				emitElement(elementTypeEnum::Button, 0);
				vec2 p = base->contentPosition;
				vec2 s = base->contentSize;
				offset(p, s, -skin().defaults.button.padding - skin().layouts[(uint32)elementTypeEnum::Button].border);
				// todo image
				if (base->text)
					base->text->emit(p, s);
			}

			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				makeFocused();
				if (buttons != mouseButtonsFlags::Left)
					return true;
				if (modifiers != modifiersFlags::None)
					return true;
				base->impl->widgetEvent.dispatch(base->entity->getName());
				return true;
			}
		};
	}

	void buttonCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<buttonImpl>(item);
	}
}
