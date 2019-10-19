#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include "../private.h"

namespace cage
{
	namespace
	{
		struct labelImpl : public widgetItemStruct
		{
			labelImpl(hierarchyItemStruct *hierarchy) : widgetItemStruct(hierarchy)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT(!hierarchy->firstChild, "label may not have children");
				CAGE_ASSERT(!!hierarchy->text || !!hierarchy->image, "label must have text or image");
				if (hierarchy->text)
					hierarchy->text->text.apply(skin->defaults.label.textFormat, hierarchy->impl);
				if (hierarchy->image)
					hierarchy->image->apply(skin->defaults.label.imageFormat);
			}

			virtual void findRequestedSize() override
			{
				hierarchy->requestedSize = vec2();
				if (hierarchy->text)
					hierarchy->requestedSize = max(hierarchy->requestedSize, hierarchy->text->findRequestedSize());
				else if (hierarchy->image)
					hierarchy->requestedSize = max(hierarchy->requestedSize, hierarchy->image->findRequestedSize());
				else
					CAGE_ASSERT(false);
				offsetSize(hierarchy->requestedSize, skin->defaults.label.margin);
			}

			virtual void emit() const override
			{
				vec2 p = hierarchy->renderPos;
				vec2 s = hierarchy->renderSize;
				offset(p, s, -skin->defaults.label.margin);
				if (hierarchy->image)
					hierarchy->image->emit(p, s);
				if (hierarchy->text)
					hierarchy->text->emit(p, s);
			}

			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				return false;
			}

			virtual bool mouseDouble(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				return false;
			}

			virtual bool mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				return false;
			}

			virtual bool mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				return false;
			}

			virtual bool mouseWheel(sint8 wheel, modifiersFlags modifiers, vec2 point) override
			{
				return false;
			}
		};
	}

	void labelCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<labelImpl>(item);
	}
}
