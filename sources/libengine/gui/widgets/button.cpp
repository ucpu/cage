#include "../private.h"

namespace cage
{
	namespace
	{
		struct ButtonImpl : public WidgetItem
		{
			ButtonImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.empty());
				if (hierarchy->text)
					hierarchy->text->apply(skin->defaults.button.textFormat);
			}

			virtual void findRequestedSize() override
			{
				hierarchy->requestedSize = skin->defaults.button.size;
				offsetSize(hierarchy->requestedSize, skin->defaults.button.margin);
			}

			virtual void emit() override
			{
				{
					vec2 p = hierarchy->renderPos;
					vec2 s = hierarchy->renderSize;
					offset(p, s, -skin->defaults.button.margin);
					emitElement(GuiElementTypeEnum::Button, mode(), p, s);
				}
				{
					vec2 p = hierarchy->renderPos;
					vec2 s = hierarchy->renderSize;
					offset(p, s, -skin->defaults.button.margin - skin->layouts[(uint32)GuiElementTypeEnum::Button].border - skin->defaults.button.padding);
					if (hierarchy->image)
						hierarchy->image->emit(p, s);
					if (hierarchy->text)
						hierarchy->text->emit(p, s);
				}
			}

			virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override
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
