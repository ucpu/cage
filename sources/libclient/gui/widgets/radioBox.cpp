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
		struct radioBoxImpl : public widgetBaseStruct
		{
			radioBoxComponent &data;

			radioBoxImpl(guiItemStruct *base) : widgetBaseStruct(base), data(GUI_REF_COMPONENT(radioBox))
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(!base->firstChild, "radiobox may not have children");
				CAGE_ASSERT_RUNTIME(!base->layout, "radiobox may not have layout");
				CAGE_ASSERT_RUNTIME(!base->image, "radiobox may not have image");
				if (base->text)
					base->text->text.apply(skin().defaults.radioBox.textFormat, base->impl);
			}

			virtual void findRequestedSize() override
			{
				base->requestedSize = skin().defaults.radioBox.size;
				if (base->text)
				{
					vec2 txtSize = base->text->findRequestedSize() + skin().defaults.radioBox.labelOffset;
					base->requestedSize[0] += txtSize[0];
					base->requestedSize[1] = max(base->requestedSize[1], txtSize[1]);
				}
				offsetSize(base->requestedSize, skin().defaults.radioBox.margin);
			}

			virtual void emit() const override
			{
				vec2 sd = skin().defaults.radioBox.size;
				{
					vec2 p = base->renderPos;
					offsetPosition(p, -skin().defaults.radioBox.margin);
					emitElement(elementTypeEnum((uint32)elementTypeEnum::RadioBoxUnchecked + (uint32)data.state), mode(p, sd), p, sd);
				}
				if (base->text)
				{
					vec2 p = base->renderPos;
					vec2 s = base->renderSize;
					offset(p, s, -skin().defaults.radioBox.margin);
					vec2 o = sd * vec2(1, 0) + skin().defaults.radioBox.labelOffset;
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
				base->impl->widgetEvent.dispatch(base->entity->name());
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

	void radioBoxCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<radioBoxImpl>(item);
	}
}
