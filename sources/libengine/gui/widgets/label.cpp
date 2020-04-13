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
				CAGE_ASSERT(!hierarchy->firstChild);
				CAGE_ASSERT(!!hierarchy->text || !!hierarchy->Image);
				if (hierarchy->text)
					hierarchy->text->text.apply(skin->defaults.label.textFormat, hierarchy->impl);
				if (hierarchy->Image)
					hierarchy->Image->apply(skin->defaults.label.imageFormat);
			}

			virtual void findRequestedSize() override
			{
				hierarchy->requestedSize = vec2();
				if (hierarchy->text)
					hierarchy->requestedSize = max(hierarchy->requestedSize, hierarchy->text->findRequestedSize());
				else if (hierarchy->Image)
					hierarchy->requestedSize = max(hierarchy->requestedSize, hierarchy->Image->findRequestedSize());
				else
				{
					CAGE_ASSERT(false);
				}
				offsetSize(hierarchy->requestedSize, skin->defaults.label.margin);
			}

			virtual void emit() const override
			{
				vec2 p = hierarchy->renderPos;
				vec2 s = hierarchy->renderSize;
				offset(p, s, -skin->defaults.label.margin);
				if (hierarchy->Image)
					hierarchy->Image->emit(p, s);
				if (hierarchy->text)
					hierarchy->text->emit(p, s);
			}

			virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override
			{
				return false;
			}

			virtual bool mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override
			{
				return false;
			}

			virtual bool mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override
			{
				return false;
			}

			virtual bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override
			{
				return false;
			}

			virtual bool mouseWheel(sint8 wheel, ModifiersFlags modifiers, vec2 point) override
			{
				return false;
			}
		};
	}

	void LabelCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<LabelImpl>(item);
	}
}
