#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/memory.h>
#include <cage-core/assets.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
#include <cage-client/assets.h>
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
				{
					const auto &d = skin().defaults.label.textFormat;
					auto &t = base->text->text;
					t.font = base->impl->assetManager->tryGet<assetSchemeIndexFont, fontClass>(d.fontName);
					t.color = d.color;
					auto &f = t.format;
					f.align = d.align;
					f.lineSpacing = d.lineSpacing;
				}
				if (base->image)
				{
					const auto &d = skin().defaults.label.imageFormat;
					// todo
				}
			}

			virtual void updateRequestedSize() override
			{
				if (base->text)
					base->text->updateRequestedSize();
				else if (base->image)
					base->image->updateRequestedSize();
				else
					CAGE_ASSERT_RUNTIME(false);
				sizeOffset(base->requestedSize, skin().defaults.label.margin);
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{
				base->contentPosition = base->position;
				base->contentSize = base->size;
				positionOffset(base->contentPosition, -skin().defaults.label.margin);
				sizeOffset(base->contentSize, -skin().defaults.label.margin);
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
