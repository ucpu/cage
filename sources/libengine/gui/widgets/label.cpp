#include "../private.h"

namespace cage
{
	namespace
	{
		struct LabelImpl : public WidgetItem
		{
			LabelImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.empty());
				CAGE_ASSERT(!!hierarchy->text || !!hierarchy->image);
				if (hierarchy->text)
					hierarchy->text->apply(skin->defaults.label.textFormat);
				if (hierarchy->image)
					hierarchy->image->apply(skin->defaults.label.imageFormat);
			}

			virtual void findRequestedSize() override
			{
				hierarchy->requestedSize = Vec2();
				if (hierarchy->text)
					hierarchy->requestedSize = max(hierarchy->requestedSize, hierarchy->text->findRequestedSize());
				else if (hierarchy->image)
					hierarchy->requestedSize = max(hierarchy->requestedSize, hierarchy->image->findRequestedSize());
				else
				{
					CAGE_ASSERT(false);
				}
				offsetSize(hierarchy->requestedSize, skin->defaults.label.margin);
			}

			virtual void emit() override
			{
				Vec2 p = hierarchy->renderPos;
				Vec2 s = hierarchy->renderSize;
				offset(p, s, -skin->defaults.label.margin);
				if (hierarchy->image)
					hierarchy->image->emit(p, s);
				if (hierarchy->text)
					hierarchy->text->emit(p, s);
			}

			virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override
			{
				return false;
			}

			virtual bool mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override
			{
				return false;
			}

			virtual bool mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override
			{
				return false;
			}

			virtual bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override
			{
				return false;
			}

			virtual bool mouseWheel(sint8 wheel, ModifiersFlags modifiers, Vec2 point) override
			{
				return false;
			}
		};
	}

	void LabelCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<LabelImpl>(item).cast<BaseItem>();
	}
}
