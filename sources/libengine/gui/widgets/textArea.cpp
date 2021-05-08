#include "../private.h"

namespace cage
{
	namespace
	{
		struct TextAreaImpl : public WidgetItem
		{
			TextAreaImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy)
			{}

			virtual void initialize() override
			{

			}

			virtual void findRequestedSize() override
			{
				hierarchy->requestedSize = vec2(); // todo this is a temporary hack
			}

			virtual void findFinalPosition(const FinalPosition &update) override
			{

			}

			virtual void emit() override
			{

			}
		};
	}

	void TextAreaCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<TextAreaImpl>(item).cast<BaseItem>();
	}
}
