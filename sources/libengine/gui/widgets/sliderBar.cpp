#include "../private.h"

namespace cage
{
	namespace
	{
		struct SliderBarImpl : public WidgetItem
		{
			GuiSliderBarComponent &data;
			GuiSkinWidgetDefaults::SliderBar::Direction defaults;
			GuiElementTypeEnum baseElement = GuiElementTypeEnum::InvalidElement;
			GuiElementTypeEnum dotElement = GuiElementTypeEnum::InvalidElement;
			Real normalizedValue;

			SliderBarImpl(HierarchyItem *hierarchy) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(SliderBar)) { data.value = clamp(data.value, data.min, data.max); }

			void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.empty());
				CAGE_ASSERT(!hierarchy->text);
				CAGE_ASSERT(!hierarchy->image);
				CAGE_ASSERT(data.value.valid() && data.min.valid() && data.max.valid());
				CAGE_ASSERT(data.max > data.min);
				normalizedValue = (data.value - data.min) / (data.max - data.min);
			}

			void findRequestedSize(Real maxWidth) override
			{
				defaults = data.vertical ? skin->defaults.sliderBar.vertical : skin->defaults.sliderBar.horizontal;
				baseElement = data.vertical ? GuiElementTypeEnum::SliderVerticalPanel : GuiElementTypeEnum::SliderHorizontalPanel;
				dotElement = data.vertical ? GuiElementTypeEnum::SliderVerticalDot : GuiElementTypeEnum::SliderHorizontalDot;
				hierarchy->requestedSize = defaults.size;
				offsetSize(hierarchy->requestedSize, defaults.margin);
			}

			void emit() override
			{
				Vec2 p = hierarchy->renderPos;
				Vec2 s = hierarchy->renderSize;
				offset(p, s, -defaults.margin);
				{
					Vec2 bp = p, bs = s;
					if (defaults.collapsedBar)
					{
						Vec2 fs;
						offsetSize(fs, skin->layouts[(uint32)baseElement].border);
						Vec2 diff = (bs - fs) / 2;
						Real m = min(diff[0], diff[1]);
						offset(bp, bs, -Vec4(m));
					}
					emitElement(baseElement, mode(), bp, bs);
				}
				offset(p, s, -skin->layouts[(uint32)baseElement].border);
				offset(p, s, -defaults.padding);
				Vec2 ds = Vec2(min(s[0], s[1]));
				Vec2 inner = s - ds;
				Vec2 dp = Vec2(interpolate(0, inner[0], normalizedValue), interpolate(inner[1], 0, normalizedValue)) + p;
				emitElement(dotElement, mode(), dp, ds);
			}

			bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override
			{
				CAGE_ASSERT(buttons != MouseButtonsFlags::None);
				makeFocused();
				Vec2 p = hierarchy->renderPos;
				Vec2 s = hierarchy->renderSize;
				offset(p, s, -defaults.margin - skin->layouts[(uint32)baseElement].border);
				if (s[0] == s[1])
					return true;
				Real ds1 = min(s[0], s[1]);
				Real mp = point[data.vertical];
				Real cp = p[data.vertical];
				Real cs = s[data.vertical];
				mp -= ds1 * 0.5;
				cs -= ds1;
				if (cs > 0)
				{
					Real f = (mp - cp) / cs;
					f = clamp(f, 0, 1);
					if (data.vertical)
						f = 1 - f;
					data.value = f * (data.max - data.min) + data.min;
					playExclusive(skin->defaults.sliderBar.slidingSound);
					hierarchy->fireWidgetEvent(input::GuiValue{ hierarchy->impl, hierarchy->ent, buttons, modifiers });
				}
				return true;
			}

			bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override
			{
				if (hasFocus() && buttons != MouseButtonsFlags::None)
					return mousePress(buttons, modifiers, point);
				return false;
			}
		};
	}

	void SliderBarCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<SliderBarImpl>(item).cast<BaseItem>();
	}
}
