#include "../private.h"

namespace cage
{
	namespace
	{
		struct CheckBoxImpl : public WidgetItem
		{
			GuiCheckBoxComponent &data;
			GuiElementTypeEnum element;

			CheckBoxImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(CheckBox)), element(GuiElementTypeEnum::TotalElements)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.empty());
				CAGE_ASSERT(!hierarchy->image);
				if (hierarchy->text)
					hierarchy->text->apply(skin->defaults.checkBox.textFormat);
				element = GuiElementTypeEnum((uint32)GuiElementTypeEnum::CheckBoxUnchecked + (uint32)data.state);
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

			virtual void emit() override
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

	void CheckBoxCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<CheckBoxImpl>(item).cast<BaseItem>();
	}
}
