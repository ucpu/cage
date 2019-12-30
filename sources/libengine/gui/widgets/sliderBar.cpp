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
		struct sliderBarImpl : public widgetItemStruct
		{
			SliderBarComponent &data;
			GuiSkinWidgetDefaults::SliderBar::Direction defaults;
			GuiElementTypeEnum baseElement;
			GuiElementTypeEnum dotElement;
			real normalizedValue;

			sliderBarImpl(hierarchyItemStruct *hierarchy) : widgetItemStruct(hierarchy), data(GUI_REF_COMPONENT(SliderBar))
			{
				data.value = clamp(data.value, data.min, data.max);
			}

			virtual void initialize() override
			{
				CAGE_ASSERT(!hierarchy->firstChild, "slider may not have children");
				CAGE_ASSERT(!hierarchy->text, "slider may not have text");
				CAGE_ASSERT(!hierarchy->Image, "slider may not have image");
				CAGE_ASSERT(data.value.valid() && data.min.valid() && data.max.valid());
				CAGE_ASSERT(data.max > data.min);
				normalizedValue = (data.value - data.min) / (data.max - data.min);
			}

			virtual void findRequestedSize() override
			{
				defaults = data.vertical ? skin->defaults.sliderBar.vertical : skin->defaults.sliderBar.horizontal;
				baseElement = data.vertical ? GuiElementTypeEnum::SliderVerticalPanel : GuiElementTypeEnum::SliderHorizontalPanel;
				dotElement = data.vertical ? GuiElementTypeEnum::SliderVerticalDot : GuiElementTypeEnum::SliderHorizontalDot;
				hierarchy->requestedSize = defaults.size;
				offsetSize(hierarchy->requestedSize, defaults.margin);
			}

			virtual void emit() const override
			{
				vec2 p = hierarchy->renderPos;
				vec2 s = hierarchy->renderSize;
				offset(p, s, -defaults.margin);
				{
					vec2 bp = p, bs = s;
					if (defaults.collapsedBar)
					{
						vec2 fs;
						offsetSize(fs, skin->layouts[(uint32)baseElement].border);
						vec2 diff = (bs - fs) / 2;
						real m = min(diff[0], diff[1]);
						diff = vec2(m, m);
						offset(bp, bs, -vec4(diff, diff));
					}
					emitElement(baseElement, mode(), bp, bs);
				}
				offset(p, s, -skin->layouts[(uint32)baseElement].border);
				offset(p, s, -defaults.padding);
				real f = normalizedValue;
				real ds1 = min(s[0], s[1]);
				vec2 ds = vec2(ds1, ds1);
				vec2 inner = s - ds;
				vec2 dp = vec2(interpolate(0, inner[0], f), interpolate(inner[1], 0, f)) + p;
				emitElement(dotElement, mode(), dp, ds);
			}

			void update(vec2 point)
			{
				vec2 p = hierarchy->renderPos;
				vec2 s = hierarchy->renderSize;
				offset(p, s, -defaults.margin - skin->layouts[(uint32)baseElement].border);
				if (s[0] == s[1])
					return;
				real ds1 = min(s[0], s[1]);
				real mp = point[data.vertical];
				real cp = p[data.vertical];
				real cs = s[data.vertical];
				mp -= ds1 * 0.5;
				cs -= ds1;
				if (cs > 0)
				{
					real f = (mp - cp) / cs;
					f = clamp(f, 0, 1);
					if (data.vertical)
						f = 1 - f;
					data.value = f * (data.max - data.min) + data.min;
					hierarchy->fireWidgetEvent();
				}
			}

			virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override
			{
				makeFocused();
				if (buttons != MouseButtonsFlags::Left)
					return true;
				if (modifiers != ModifiersFlags::None)
					return true;
				update(point);
				return true;
			}

			virtual bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override
			{
				if (hasFocus())
					return mousePress(buttons, modifiers, point);
				return false;
			}
		};
	}

	void SliderBarCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<sliderBarImpl>(item);
	}
}
