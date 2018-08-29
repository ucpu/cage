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
		struct checkBoxImpl : public widgetBaseStruct
		{
			checkBoxComponent &data;

			checkBoxImpl(guiItemStruct *base) : widgetBaseStruct(base), data(GUI_REF_COMPONENT(checkBox))
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(!base->firstChild, "checkbox may not have children");
				CAGE_ASSERT_RUNTIME(!base->layout, "checkbox may not have layout");
				CAGE_ASSERT_RUNTIME(!base->image, "checkbox may not have image");
				if (base->text)
					base->text->text.apply(skin().defaults.checkBox.textFormat, base->impl);
			}

			virtual void findRequestedSize() override
			{
				base->requestedSize = skin().defaults.checkBox.size;
				if (base->text)
				{
					vec2 txtSize = base->text->findRequestedSize() + skin().defaults.checkBox.labelOffset;
					base->requestedSize[0] += txtSize[0];
					base->requestedSize[1] = max(base->requestedSize[1], txtSize[1]);
				}
				offsetSize(base->requestedSize, skin().defaults.checkBox.margin);
			}

			virtual void emit() const override
			{
				vec2 sd = skin().defaults.checkBox.size;
				{
					vec2 p = base->position;
					offsetPosition(p, -skin().defaults.checkBox.margin);
					elementTypeEnum el = data.type == checkBoxTypeEnum::RadioButton ? elementTypeEnum::RadioBoxUnchecked : elementTypeEnum::CheckBoxUnchecked;
					emitElement(elementTypeEnum((uint32)el + (uint32)data.state), mode(p, sd), p, sd);
				}
				if (base->text)
				{
					vec2 p = base->position;
					vec2 s = base->size;
					offset(p, s, -skin().defaults.checkBox.margin);
					vec2 o = sd * vec2(1, 0) + skin().defaults.checkBox.labelOffset;
					p += o;
					s -= o;
					base->text->emit(p, s);
				}
			}

			void update()
			{
				if (data.state == checkBoxStateEnum::Checked)
					data.state = checkBoxStateEnum::Unchecked;
				else
					data.state = checkBoxStateEnum::Checked;
				base->impl->widgetEvent.dispatch(base->entity->getName());
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

	void checkBoxCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<checkBoxImpl>(item);
	}
}
