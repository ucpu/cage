#include "../private.h"

namespace cage
{
	namespace
	{
		struct FrameImpl : public WidgetItem
		{
			FrameImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy) { ensureItemHasLayout(hierarchy); }

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.size() == 1);
				CAGE_ASSERT(!hierarchy->text && !hierarchy->image);
			}

			void findRequestedSize() override
			{
				hierarchy->children[0]->findRequestedSize();
				hierarchy->requestedSize = hierarchy->children[0]->requestedSize;
				offsetSize(hierarchy->requestedSize, skin->defaults.frame.padding);
				offsetSize(hierarchy->requestedSize, skin->layouts[(uint32)GuiElementTypeEnum::Frame].border);
				offsetSize(hierarchy->requestedSize, skin->defaults.frame.margin);
			}

			void findFinalPosition(const FinalPosition &update) override
			{
				FinalPosition u(update);
				offset(u.renderPos, u.renderSize, -skin->defaults.frame.margin);
				offset(u.renderPos, u.renderSize, -skin->layouts[(uint32)GuiElementTypeEnum::Frame].border);
				offset(u.renderPos, u.renderSize, -skin->defaults.frame.padding);
				hierarchy->children[0]->findFinalPosition(u);
			}

			void emit() override
			{
				Vec2 p = hierarchy->renderPos;
				Vec2 s = hierarchy->renderSize;
				offset(p, s, -skin->defaults.frame.margin);
				emitElement(GuiElementTypeEnum::Frame, mode(false, 0), p, s);
				hierarchy->childrenEmit();
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
		};
	}

	void FrameCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<FrameImpl>(item).cast<BaseItem>();
	}
}
