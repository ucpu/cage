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
		struct RadioBoxImpl : public WidgetItem
		{
			GuiRadioBoxComponent &data;
			GuiElementTypeEnum element;

			RadioBoxImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(RadioBox)), element(GuiElementTypeEnum::TotalElements)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT(!hierarchy->firstChild, "radiobox may not have children");
				CAGE_ASSERT(!hierarchy->Image, "radiobox may not have image");
				if (hierarchy->text)
					hierarchy->text->text.apply(skin->defaults.radioBox.textFormat, hierarchy->impl);
				element = GuiElementTypeEnum((uint32)GuiElementTypeEnum::RadioBoxUnchecked + (uint32)data.state);
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
				if (data.state == CheckBoxStateEnum::Checked)
					data.state = CheckBoxStateEnum::Unchecked;
				else
					data.state = CheckBoxStateEnum::Checked;
				hierarchy->fireWidgetEvent();
			}

			virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override
			{
				makeFocused();
				if (buttons != MouseButtonsFlags::Left)
					return true;
				if (modifiers != ModifiersFlags::None)
					return true;
				update();
				return true;
			}
		};
	}

	void RadioBoxCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<RadioBoxImpl>(item);
	}
}
