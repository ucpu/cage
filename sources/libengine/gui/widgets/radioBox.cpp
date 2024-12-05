#include "../private.h"

namespace cage
{
	namespace
	{
		struct RadioBoxImpl : public WidgetItem
		{
			GuiRadioBoxComponent &data;
			GuiElementTypeEnum element = GuiElementTypeEnum::InvalidElement;

			RadioBoxImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(RadioBox)) {}

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.empty());
				CAGE_ASSERT(!hierarchy->image);
				if (hierarchy->text)
					hierarchy->text->apply(skin->defaults.radioBox.textFormat);
				element = GuiElementTypeEnum((uint32)GuiElementTypeEnum::RadioBoxUnchecked + (uint32)data.state);
			}

			void findRequestedSize() override
			{
				hierarchy->requestedSize = skin->defaults.radioBox.size;
				if (hierarchy->text)
				{
					Vec2 txtSize = hierarchy->text->findRequestedSize() + skin->defaults.radioBox.labelOffset;
					hierarchy->requestedSize[0] += txtSize[0];
					hierarchy->requestedSize[1] = max(hierarchy->requestedSize[1], txtSize[1]);
				}
				offsetSize(hierarchy->requestedSize, skin->defaults.radioBox.margin);
			}

			void emit() override
			{
				Vec2 sd = skin->defaults.radioBox.size;
				{
					Vec2 p = hierarchy->renderPos;
					offsetPosition(p, -skin->defaults.radioBox.margin);
					emitElement(element, mode(), p, sd);
				}
				if (hierarchy->text)
				{
					Vec2 p = hierarchy->renderPos;
					Vec2 s = hierarchy->renderSize;
					offset(p, s, -skin->defaults.radioBox.margin);
					Vec2 o = sd * Vec2(1, 0) + skin->defaults.radioBox.labelOffset;
					p += o;
					s -= o;
					hierarchy->text->emit(p, s, widgetState.disabled);
				}
			}

			static void deselect(HierarchyItem *item)
			{
				if (RadioBoxImpl *b = dynamic_cast<RadioBoxImpl *>(+item->item))
					b->data.state = CheckBoxStateEnum::Unchecked;
			}

			bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override
			{
				CAGE_ASSERT(buttons != MouseButtonsFlags::None);
				makeFocused();
				HierarchyItem *parent = hierarchy->impl->root->findParentOf(hierarchy);
				for (const auto &it : parent->children)
					deselect(+it);
				data.state = CheckBoxStateEnum::Checked;
				hierarchy->fireWidgetEvent(input::GuiValue{ hierarchy->impl, hierarchy->ent, buttons, modifiers });
				return true;
			}
		};
	}

	void RadioBoxCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<RadioBoxImpl>(item).cast<BaseItem>();
	}
}
