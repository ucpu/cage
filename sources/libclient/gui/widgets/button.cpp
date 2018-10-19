#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "../private.h"

namespace cage
{
	namespace
	{
		struct buttonImpl : public widgetItemStruct
		{
			buttonImpl(hierarchyItemStruct *hierarchy) : widgetItemStruct(hierarchy)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(!hierarchy->firstChild, "button may not have children");
				if (hierarchy->text)
					hierarchy->text->text.apply(skin->defaults.button.textFormat, hierarchy->impl);
			}

			virtual void findRequestedSize() override
			{
				hierarchy->requestedSize = skin->defaults.button.size;
				offsetSize(hierarchy->requestedSize, skin->defaults.button.margin);
			}

			virtual void emit() const override
			{
				{
					vec2 p = hierarchy->renderPos;
					vec2 s = hierarchy->renderSize;
					offset(p, s, -skin->defaults.button.margin);
					emitElement(elementTypeEnum::Button, mode(), p, s);
				}
				{
					vec2 p = hierarchy->renderPos;
					vec2 s = hierarchy->renderSize;
					offset(p, s, -skin->defaults.button.margin - skin->layouts[(uint32)elementTypeEnum::Button].border - skin->defaults.button.padding);
					if (hierarchy->image)
						hierarchy->image->emit(p, s);
					if (hierarchy->text)
						hierarchy->text->emit(p, s);
				}
			}

			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				makeFocused();
				if (buttons != mouseButtonsFlags::Left)
					return true;
				if (modifiers != modifiersFlags::None)
					return true;
				hierarchy->impl->widgetEvent.dispatch(hierarchy->entity->name());
				return true;
			}
		};
	}

	void buttonCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT_RUNTIME(!item->item);
		item->item = item->impl->itemsMemory.createObject<buttonImpl>(item);
	}
}
