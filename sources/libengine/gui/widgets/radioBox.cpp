#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include "../private.h"

namespace cage
{
	namespace
	{
		struct radioBoxImpl : public widgetItemStruct
		{
			radioBoxComponent &data;
			elementTypeEnum element;

			radioBoxImpl(hierarchyItemStruct *hierarchy) : widgetItemStruct(hierarchy), data(GUI_REF_COMPONENT(radioBox)), element(elementTypeEnum::TotalElements)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT(!hierarchy->firstChild, "radiobox may not have children");
				CAGE_ASSERT(!hierarchy->Image, "radiobox may not have image");
				if (hierarchy->text)
					hierarchy->text->text.apply(skin->defaults.radioBox.textFormat, hierarchy->impl);
				element = elementTypeEnum((uint32)elementTypeEnum::RadioBoxUnchecked + (uint32)data.state);
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
					emitElement(element, mode(), p, sd);
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
				hierarchy->fireWidgetEvent();
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
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<radioBoxImpl>(item);
	}
}
