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
		struct radioBoxImpl : public widgetItemStruct
		{
			radioBoxComponent &data;

			radioBoxImpl(hierarchyItemStruct *hierarchy) : widgetItemStruct(hierarchy), data(GUI_REF_COMPONENT(radioBox))
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(!hierarchy->firstChild, "radiobox may not have children");
				CAGE_ASSERT_RUNTIME(!hierarchy->image, "radiobox may not have image");
				if (hierarchy->text)
					hierarchy->text->text.apply(skin->defaults.radioBox.textFormat, hierarchy->impl);
			}

			virtual void findRequestedSize() override
			{
				hierarchy->requestedSize = skin->defaults.radioBox.size;
				if (hierarchy->text)
				{
					vec2 txtSize = hierarchy->text->findRequestedSize() + skin->defaults.radioBox.labelOffset;
					hierarchy->requestedSize[0] += txtSize[0];
					hierarchy->requestedSize[1] = max(hierarchy->requestedSize[1], txtSize[1]);
				}
				offsetSize(hierarchy->requestedSize, skin->defaults.radioBox.margin);
			}

			virtual void emit() const override
			{
				vec2 sd = skin->defaults.radioBox.size;
				{
					vec2 p = hierarchy->renderPos;
					offsetPosition(p, -skin->defaults.radioBox.margin);
					emitElement(elementTypeEnum((uint32)elementTypeEnum::RadioBoxUnchecked + (uint32)data.state), mode(), p, sd);
				}
				if (hierarchy->text)
				{
					vec2 p = hierarchy->renderPos;
					vec2 s = hierarchy->renderSize;
					offset(p, s, -skin->defaults.radioBox.margin);
					vec2 o = sd * vec2(1, 0) + skin->defaults.radioBox.labelOffset;
					p += o;
					s -= o;
					hierarchy->text->emit(p, s);
				}
			}

			void update()
			{
				if (data.state == checkBoxStateEnum::Checked)
					data.state = checkBoxStateEnum::Unchecked;
				else
					data.state = checkBoxStateEnum::Checked;
				hierarchy->impl->widgetEvent.dispatch(hierarchy->entity->name());
			}

			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				makeFocused();
				if (buttons != mouseButtonsFlags::Left)
					return true;
				if (modifiers != modifiersFlags::None)
					return true;
				update();
				return true;
			}
		};
	}

	void radioBoxCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT_RUNTIME(!item->item);
		item->item = item->impl->itemsMemory.createObject<radioBoxImpl>(item);
	}
}
