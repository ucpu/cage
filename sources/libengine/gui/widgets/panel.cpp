#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

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
		struct panelImpl : public widgetItemStruct
		{
			panelComponent &data;

			panelImpl(hierarchyItemStruct *hierarchy) : widgetItemStruct(hierarchy), data(GUI_REF_COMPONENT(panel))
			{
				ensureItemHasLayout(hierarchy);
			}

			virtual void initialize() override
			{
				if (hierarchy->text)
					hierarchy->text->text.apply(skin->defaults.panel.textFormat, hierarchy->impl);
			}

			virtual void findRequestedSize() override
			{
				hierarchy->firstChild->findRequestedSize();
				hierarchy->requestedSize = hierarchy->firstChild->requestedSize;
				offsetSize(hierarchy->requestedSize, skin->defaults.panel.contentPadding);
				if (hierarchy->text)
				{
					hierarchy->requestedSize[1] += skin->defaults.panel.captionHeight;
					vec2 cs = hierarchy->text->findRequestedSize();
					offsetSize(cs, skin->defaults.panel.captionPadding);
					hierarchy->requestedSize[0] = max(hierarchy->requestedSize[0], cs[0]);
					// it is important to compare (text size + text padding) with (children size + children padding)
					// and only after that to add border and base margin
				}
				offsetSize(hierarchy->requestedSize, skin->layouts[(uint32)elementTypeEnum::PanelBase].border);
				offsetSize(hierarchy->requestedSize, skin->defaults.panel.baseMargin);
			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{
				finalPositionStruct u(update);
				offset(u.renderPos, u.renderSize, -skin->defaults.panel.baseMargin);
				offset(u.renderPos, u.renderSize, -skin->layouts[(uint32)elementTypeEnum::PanelBase].border);
				if (hierarchy->text)
				{
					u.renderPos[1] += skin->defaults.panel.captionHeight;
					u.renderSize[1] -= skin->defaults.panel.captionHeight;
				}
				offset(u.renderPos, u.renderSize, -skin->defaults.panel.contentPadding);
				hierarchy->firstChild->findFinalPosition(u);
			}

			virtual void emit() const override
			{
				vec2 p = hierarchy->renderPos;
				vec2 s = hierarchy->renderSize;
				offset(p, s, -skin->defaults.panel.baseMargin);
				emitElement(elementTypeEnum::PanelBase, mode(false, 0), p, s);
				if (hierarchy->text)
				{
					s = vec2(s[0], skin->defaults.panel.captionHeight);
					emitElement(elementTypeEnum::PanelCaption, mode(false, 0), p, s);
					offset(p, s, -skin->layouts[(uint32)elementTypeEnum::PanelCaption].border);
					offset(p, s, -skin->defaults.panel.captionPadding);
					hierarchy->text->emit(p, s);
				}
				hierarchy->childrenEmit();
			}
		};
	}

	void panelCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<panelImpl>(item);
	}
}
