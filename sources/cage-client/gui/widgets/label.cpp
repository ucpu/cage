#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
#include "../private.h"

namespace cage
{
	namespace
	{
		struct labelImpl : public widgetBaseStruct
		{
			labelImpl(guiItemStruct *base) : widgetBaseStruct(base)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(!base->firstChild, "label may not have children");
				CAGE_ASSERT_RUNTIME(!base->layout, "label may not have layout");
				CAGE_ASSERT_RUNTIME(!!base->text != !!base->image, "label must have text xor image");
				if (base->text)
					base->text->text.apply(skin().defaults.label.textFormat, base->impl);
				if (base->image)
				{
					const auto &d = skin().defaults.label.imageFormat;
					// todo
				}
			}

			virtual void updateRequestedSize() override
			{
				if (base->text)
					base->requestedSize = base->text->updateRequestedSize();
				else if (base->image)
					base->requestedSize = base->image->updateRequestedSize();
				else
					CAGE_ASSERT_RUNTIME(false);
				offsetSize(base->requestedSize, skin().defaults.label.margin);
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{
				base->updateContentPosition(skin().defaults.label.margin);
			}

			virtual void emit() const override
			{
				if (base->image)
					base->image->emit();
				if (base->text)
					base->text->emit();
			}
		};
	}

	void labelCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<labelImpl>(item);
	}
}
