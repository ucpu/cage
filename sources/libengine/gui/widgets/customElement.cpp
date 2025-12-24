#include "../private.h"

namespace cage
{
	void guiSubtract(Real &maxWidth, const Vec4 &sizes);

	namespace
	{
		struct CustomElementImpl : public WidgetItem
		{
			const GuiCustomElementComponent &data;

			CustomElementImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(CustomElement)) { ensureItemHasLayout(hierarchy); }

			void initialize() override
			{
				CAGE_ASSERT(data.element < GuiElementTypeEnum::TotalElements);
				CAGE_ASSERT(hierarchy->children.size() == 1);
				CAGE_ASSERT(!hierarchy->text && !hierarchy->image);
			}

			void findRequestedSize(Real maxWidth) override
			{
				guiSubtract(maxWidth, data.padding);
				guiSubtract(maxWidth, skin->layouts[(uint32)data.element].border);
				guiSubtract(maxWidth, data.margin);
				hierarchy->children[0]->findRequestedSize(maxWidth);
				hierarchy->requestedSize = hierarchy->children[0]->requestedSize;
				offsetSize(hierarchy->requestedSize, data.padding);
				offsetSize(hierarchy->requestedSize, skin->layouts[(uint32)data.element].border);
				offsetSize(hierarchy->requestedSize, data.margin);
			}

			void findFinalPosition(const FinalPosition &update) override
			{
				FinalPosition u(update);
				offset(u.renderPos, u.renderSize, -data.margin);
				offset(u.renderPos, u.renderSize, -skin->layouts[(uint32)data.element].border);
				offset(u.renderPos, u.renderSize, -data.padding);
				hierarchy->children[0]->findFinalPosition(u);
			}

			void emit() override
			{
				Vec2 p = hierarchy->renderPos;
				Vec2 s = hierarchy->renderSize;
				offset(p, s, -data.margin);
				emitElement(data.element, mode(false, 0), p, s);
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

	void CustomElementCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<CustomElementImpl>(item).cast<BaseItem>();
	}
}
