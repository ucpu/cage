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
		struct spoilerImpl : public widgetBaseStruct
		{
			spoilerComponent &data;

			spoilerImpl(guiItemStruct *base) : widgetBaseStruct(base), data(GUI_REF_COMPONENT(spoiler))
			{}

			virtual void initialize() override
			{
				if (data.collapsed)
					base->detachChildren();
				if (base->text)
					base->text->text.apply(skin().defaults.spoiler.textFormat, base->impl);
				if (!base->layout)
					layoutDefaultCreate(base);
			}

			virtual void findRequestedSize() override
			{
				base->layout->findRequestedSize();
				offsetSize(base->requestedSize, skin().defaults.spoiler.contentPadding);
				base->requestedSize[1] += skin().defaults.spoiler.captionHeight;
				vec2 cs = base->text ? base->text->findRequestedSize() : vec2();
				offsetSize(cs, skin().defaults.spoiler.captionPadding);
				base->requestedSize[0] = max(base->requestedSize[0], cs[0]);
				offsetSize(base->requestedSize, skin().layouts[(uint32)elementTypeEnum::PanelBase].border);
				offsetSize(base->requestedSize, skin().defaults.spoiler.baseMargin);
			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{
				finalPositionStruct u(update);
				u.position[1] += skin().defaults.spoiler.captionHeight;
				offset(u.position, u.size, -skin().layouts[(uint32)elementTypeEnum::PanelBase].border);
				offset(u.position, u.size, -skin().defaults.spoiler.baseMargin - skin().defaults.spoiler.contentPadding);
				base->layout->findFinalPosition(u);
			}

			virtual void emit() const override
			{
				vec2 p = base->position;
				vec2 s = base->size;
				offset(p, s, -skin().defaults.spoiler.baseMargin);
				emitElement(elementTypeEnum::PanelBase, mode(false), p, s);
				s = vec2(s[0], skin().defaults.spoiler.captionHeight);
				emitElement(elementTypeEnum::PanelCaption, mode(p, s), p, s);
				offset(p, s, -skin().layouts[(uint32)elementTypeEnum::PanelCaption].border - skin().defaults.spoiler.captionPadding);
				if (base->text)
					base->text->emit(p, s);
				base->childrenEmit();
				// todo emit spoiler icon
			}

			void collapse(guiItemStruct *item)
			{
				if (!item->widget)
					return;
				spoilerImpl *b = dynamic_cast<spoilerImpl*>(item->widget);
				if (!b)
					return;
				b->data.collapsed = true;
			}

			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				base->impl->focusName = 0;
				if (buttons != mouseButtonsFlags::Left)
					return true;
				if (modifiers != modifiersFlags::None)
					return true;
				vec2 p = base->position;
				vec2 s = vec2(base->size[0], skin().defaults.spoiler.captionHeight);
				offset(p, s, -skin().defaults.spoiler.baseMargin * vec4(1, 1, 1, 0));
				if (pointInside(p, s, point))
				{
					data.collapsed = !data.collapsed;
					if (data.collapsesSiblings)
					{
						guiItemStruct *i = base->prevSibling;
						while (i)
						{
							collapse(i);
							i = i->prevSibling;
						}
						i = base->nextSibling;
						while (i)
						{
							collapse(i);
							i = i->nextSibling;
						}
					}
				}
				return true;
			}
		};
	}

	void spoilerCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<spoilerImpl>(item);
	}
}
