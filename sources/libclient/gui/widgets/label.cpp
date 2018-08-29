#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
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
				CAGE_ASSERT_RUNTIME(!!base->text || !!base->image, "label must have text or image");
				if (base->text)
					base->text->text.apply(skin().defaults.label.textFormat, base->impl);
				if (base->image)
					base->image->apply(skin().defaults.label.imageFormat);
			}

			virtual void findRequestedSize() override
			{
				base->requestedSize = vec2();
				if (base->text)
					base->requestedSize = max(base->requestedSize, base->text->findRequestedSize());
				else if (base->image)
					base->requestedSize = max(base->requestedSize, base->image->findRequestedSize());
				else
					CAGE_ASSERT_RUNTIME(false);
				offsetSize(base->requestedSize, skin().defaults.label.margin);
			}

			virtual void emit() const override
			{
				vec2 p = base->position;
				vec2 s = base->size;
				offset(p, s, -skin().defaults.label.margin);
				if (base->image)
					base->image->emit(p, s);
				if (base->text)
					base->text->emit(p, s);
			}
		};
	}

	void labelCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<labelImpl>(item);
	}
}
