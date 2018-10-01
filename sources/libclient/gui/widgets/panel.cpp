#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphics.h>
#include <cage-client/window.h>
#include "../private.h"

namespace cage
{
	void layoutDefaultCreate(guiItemStruct *item);

	namespace
	{
		struct panelImpl : public widgetBaseStruct
		{
			panelComponent &data;

			panelImpl(guiItemStruct *base) : widgetBaseStruct(base), data(GUI_REF_COMPONENT(panel))
			{
				auto *impl = base->impl;
				GUI_GET_COMPONENT(panel, gb, base->entity);
				data = gb;
			}

			virtual void initialize() override
			{
				if (base->text)
					base->text->text.apply(skin().defaults.panel.textFormat, base->impl);
				if (!base->layout)
					layoutDefaultCreate(base);
			}

			virtual void findRequestedSize() override
			{
				base->layout->findRequestedSize();
				offsetSize(base->requestedSize, skin().defaults.panel.contentPadding);
				if (base->text)
				{
					base->requestedSize[1] += skin().defaults.panel.captionHeight;
					vec2 cs = base->text->findRequestedSize();
					offsetSize(cs, skin().defaults.panel.captionPadding);
					base->requestedSize[0] = max(base->requestedSize[0], cs[0]);
					// it is important to compare (text size + text padding) with (children size + children padding)
					// and only after that to add border and base margin
				}
				offsetSize(base->requestedSize, skin().layouts[(uint32)elementTypeEnum::PanelBase].border);
				offsetSize(base->requestedSize, skin().defaults.panel.baseMargin);
			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{
				finalPositionStruct u(update);
				if (base->text)
					u.position[1] += skin().defaults.panel.captionHeight;
				offset(u.position, u.size, -skin().layouts[(uint32)elementTypeEnum::PanelBase].border);
				offset(u.position, u.size, -skin().defaults.panel.baseMargin - skin().defaults.panel.contentPadding);
				base->layout->findFinalPosition(u);
			}

			virtual void emit() const override
			{
				vec2 p = base->position;
				vec2 s = base->size;
				offset(p, s, -skin().defaults.panel.baseMargin);
				emitElement(elementTypeEnum::PanelBase, mode(false, 0), p, s);
				if (base->text)
				{
					s = vec2(s[0], skin().defaults.panel.captionHeight);
					emitElement(elementTypeEnum::PanelCaption, mode(p, s, 0), p, s);
					offset(p, s, -skin().layouts[(uint32)elementTypeEnum::PanelCaption].border - skin().defaults.panel.captionPadding);
					base->text->emit(p, s);
				}
				base->childrenEmit();
			}
		};
	}

	void panelCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<panelImpl>(item);
	}
}
