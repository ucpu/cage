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
		struct checkBoxImpl : public widgetItemStruct
		{
			CheckBoxComponent &data;
			ElementTypeEnum element;

			checkBoxImpl(hierarchyItemStruct *hierarchy) : widgetItemStruct(hierarchy), data(GUI_REF_COMPONENT(CheckBox)), element(ElementTypeEnum::TotalElements)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT(!hierarchy->firstChild, "checkbox may not have children");
				CAGE_ASSERT(!hierarchy->Image, "checkbox may not have image");
				if (hierarchy->text)
					hierarchy->text->text.apply(skin->defaults.checkBox.textFormat, hierarchy->impl);
				element = ElementTypeEnum((uint32)ElementTypeEnum::CheckBoxUnchecked + (uint32)data.state);
			}

			virtual void findRequestedSize() override
			{
				hierarchy->requestedSize = skin->defaults.checkBox.size;
				if (hierarchy->text)
				{
					vec2 txtSize = hierarchy->text->findRequestedSize() + skin->defaults.checkBox.labelOffset;
					hierarchy->requestedSize[0] += txtSize[0];
					hierarchy->requestedSize[1] = max(hierarchy->requestedSize[1], txtSize[1]);
				}
				offsetSize(hierarchy->requestedSize, skin->defaults.checkBox.margin);
			}

			virtual void emit() const override
			{
				vec2 sd = skin->defaults.checkBox.size;
				{
					vec2 p = hierarchy->renderPos;
					offsetPosition(p, -skin->defaults.checkBox.margin);
					emitElement(element, mode(), p, sd);
				}
				if (hierarchy->text)
				{
					vec2 p = hierarchy->renderPos;
					vec2 s = hierarchy->renderSize;
					offset(p, s, -skin->defaults.checkBox.margin);
					vec2 o = sd * vec2(1, 0) + skin->defaults.checkBox.labelOffset;
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

	void CheckBoxCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<checkBoxImpl>(item);
	}
}
