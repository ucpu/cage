#include "../private.h"

namespace cage
{
	namespace
	{
		struct HeaderImpl : public WidgetItem
		{
			HeaderImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy) {}

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.empty());
				CAGE_ASSERT(!!hierarchy->text || !!hierarchy->image);
				if (hierarchy->text)
					hierarchy->text->apply(skin->defaults.header.textFormat);
				if (hierarchy->image)
					hierarchy->image->apply(skin->defaults.header.imageFormat);
			}

			void findRequestedSize() override
			{
				hierarchy->requestedSize = skin->defaults.header.size;
				offsetSize(hierarchy->requestedSize, skin->defaults.header.margin);
			}

			void emit() override
			{
				{
					Vec2 p = hierarchy->renderPos;
					Vec2 s = hierarchy->renderSize;
					offset(p, s, -skin->defaults.header.margin);
					emitElement(GuiElementTypeEnum::Header, mode(false), p, s);
				}
				{
					Vec2 p = hierarchy->renderPos;
					Vec2 s = hierarchy->renderSize;
					offset(p, s, -skin->defaults.header.margin - skin->layouts[(uint32)GuiElementTypeEnum::Header].border - skin->defaults.header.padding);
					if (hierarchy->image)
						hierarchy->image->emit(p, s, widgetState.disabled);
					if (hierarchy->text)
						hierarchy->text->emit(p, s, widgetState.disabled);
				}
			}

			void playHoverSound() override
			{
				// nothing
			}

			bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override
			{
				// prevent taking focus
				return false;
			}

			bool mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override { return false; }

			bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override { return false; }

			bool mouseWheel(Real wheel, ModifiersFlags modifiers, Vec2 point) override { return false; }
		};
	}

	void HeaderCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<HeaderImpl>(item).cast<BaseItem>();
	}
}
