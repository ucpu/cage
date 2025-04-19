#include "../private.h"

namespace cage
{
	namespace
	{
		struct SeparatorImpl : public WidgetItem
		{
			GuiSeparatorComponent &data;
			GuiSkinWidgetDefaults::Separator::Direction defaults;
			GuiElementTypeEnum baseElement = GuiElementTypeEnum::InvalidElement;

			SeparatorImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(Separator)) {}

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.empty());
				CAGE_ASSERT(!hierarchy->text && !hierarchy->image);
			}

			void findRequestedSize() override
			{
				defaults = data.vertical ? skin->defaults.separator.vertical : skin->defaults.separator.horizontal;
				baseElement = data.vertical ? GuiElementTypeEnum::SeparatorVerticalLine : GuiElementTypeEnum::SeparatorHorizontalLine;
				hierarchy->requestedSize = defaults.size;
				offsetSize(hierarchy->requestedSize, defaults.margin);
			}

			void emit() override
			{
				Vec2 p = hierarchy->renderPos;
				Vec2 s = hierarchy->renderSize;
				offset(p, s, -defaults.margin);
				emitElement(baseElement, mode(false), p, s);
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

	void SeparatorCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<SeparatorImpl>(item).cast<BaseItem>();
	}
}
