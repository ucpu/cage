#include "../private.h"

namespace cage
{
	namespace
	{
		struct SpacerImpl : public WidgetItem
		{
			const GuiSpacerComponent &data;

			SpacerImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(Spacer)) {}

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.empty());
				CAGE_ASSERT(!hierarchy->text && !hierarchy->image);
			}

			void findRequestedSize(Real maxWidth) override { hierarchy->requestedSize = Vec2(10); }

			void emit() override
			{
				// nothing
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

	void SpacerCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<SpacerImpl>(item).cast<BaseItem>();
	}
}
