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
		struct SpoilerImpl : public WidgetItem
		{
			GuiSpoilerComponent &data;
			bool collapsed;

			SpoilerImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(Spoiler)), collapsed(false)
			{
				ensureItemHasLayout(hierarchy);
			}

			virtual void initialize() override
			{
				collapsed = data.collapsed;
				if (collapsed)
					hierarchy->detachChildren();
				if (hierarchy->text)
					hierarchy->text->text.apply(skin->defaults.spoiler.textFormat, hierarchy->impl);
			}

			virtual void findRequestedSize() override
			{
				if (hierarchy->firstChild)
				{
					hierarchy->firstChild->findRequestedSize();
					hierarchy->requestedSize = hierarchy->firstChild->requestedSize;
				}
				else
					hierarchy->requestedSize = vec2();
				offsetSize(hierarchy->requestedSize, skin->defaults.spoiler.contentPadding);
				hierarchy->requestedSize[1] += skin->defaults.spoiler.captionHeight;
				vec2 cs = hierarchy->text ? hierarchy->text->findRequestedSize() : vec2();
				offsetSize(cs, skin->defaults.spoiler.captionPadding);
				hierarchy->requestedSize[0] = max(hierarchy->requestedSize[0], cs[0] + skin->defaults.spoiler.captionHeight);
				offsetSize(hierarchy->requestedSize, skin->layouts[(uint32)GuiElementTypeEnum::SpoilerBase].border);
				offsetSize(hierarchy->requestedSize, skin->defaults.spoiler.baseMargin);
			}

			virtual void findFinalPosition(const FinalPosition &update) override
			{
				if (!hierarchy->firstChild)
					return;
				FinalPosition u(update);
				u.renderPos[1] += skin->defaults.spoiler.captionHeight;
				u.renderSize[1] -= skin->defaults.spoiler.captionHeight;
				offset(u.renderPos, u.renderSize, -skin->layouts[(uint32)GuiElementTypeEnum::SpoilerBase].border);
				offset(u.renderPos, u.renderSize, -skin->defaults.spoiler.baseMargin - skin->defaults.spoiler.contentPadding);
				hierarchy->firstChild->findFinalPosition(u);
			}

			virtual void emit() const override
			{
				vec2 p = hierarchy->renderPos;
				vec2 s = hierarchy->renderSize;
				offset(p, s, -skin->defaults.spoiler.baseMargin);
				emitElement(GuiElementTypeEnum::SpoilerBase, mode(false), p, s);
				s = vec2(s[0], skin->defaults.spoiler.captionHeight);
				emitElement(GuiElementTypeEnum::SpoilerCaption, mode(p, s), p, s);
				offset(p, s, -skin->layouts[(uint32)GuiElementTypeEnum::SpoilerCaption].border - skin->defaults.spoiler.captionPadding);
				vec2 is = vec2(s[1], s[1]);
				vec2 ip = vec2(p[0] + s[0] - is[0], p[1]);
				emitElement(collapsed ? GuiElementTypeEnum::SpoilerIconCollapsed : GuiElementTypeEnum::SpoilerIconShown, mode(false, 0), ip, is);
				if (hierarchy->text)
				{
					s[0] -= s[1];
					s[0] = max(0, s[0]);
					hierarchy->text->emit(p, s);
				}
				hierarchy->childrenEmit();
			}

			void collapse(HierarchyItem *item)
			{
				SpoilerImpl *b = dynamic_cast<SpoilerImpl*>(item->item);
				if (!b)
					return;
				b->data.collapsed = true;
			}

			virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override
			{
				hierarchy->impl->focusName = 0;
				if (buttons != MouseButtonsFlags::Left)
					return true;
				if (modifiers != ModifiersFlags::None)
					return true;
				vec2 p = hierarchy->renderPos;
				vec2 s = vec2(hierarchy->renderSize[0], skin->defaults.spoiler.captionHeight);
				offset(p, s, -skin->defaults.spoiler.baseMargin * vec4(1, 1, 1, 0));
				if (pointInside(p, s, point))
				{
					data.collapsed = !data.collapsed;
					if (data.collapsesSiblings)
					{
						HierarchyItem *i = hierarchy->prevSibling;
						while (i)
						{
							collapse(i);
							i = i->prevSibling;
						}
						i = hierarchy->nextSibling;
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

	void SpoilerCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<SpoilerImpl>(item);
	}
}
