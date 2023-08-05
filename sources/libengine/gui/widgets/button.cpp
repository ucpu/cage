#include "../private.h"

namespace cage
{
	namespace
	{
		struct ButtonImpl : public WidgetItem
		{
			ButtonImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy) {}

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.empty());
				if (hierarchy->text)
					hierarchy->text->apply(skin->defaults.button.textFormat);
			}

			void findRequestedSize() override
			{
				hierarchy->requestedSize = skin->defaults.button.size;
				offsetSize(hierarchy->requestedSize, skin->defaults.button.margin);
			}

			void emit() override
			{
				{
					Vec2 p = hierarchy->renderPos;
					Vec2 s = hierarchy->renderSize;
					offset(p, s, -skin->defaults.button.margin);
					emitElement(GuiElementTypeEnum::Button, mode(), p, s);
				}
				{
					Vec2 p = hierarchy->renderPos;
					Vec2 s = hierarchy->renderSize;
					offset(p, s, -skin->defaults.button.margin - skin->layouts[(uint32)GuiElementTypeEnum::Button].border - skin->defaults.button.padding);
					if (hierarchy->image)
						hierarchy->image->emit(p, s, widgetState.disabled);
					if (hierarchy->text)
						hierarchy->text->emit(p, s, widgetState.disabled);
				}
			}

			bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override
			{
				makeFocused();
				if (buttons != MouseButtonsFlags::Left)
					return true;
				if (modifiers != ModifiersFlags::None)
					return true;
				hierarchy->fireWidgetEvent();
				return true;
			}
		};
	}

	void ButtonCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<ButtonImpl>(item).cast<BaseItem>();
	}
}
