#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>

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
		struct sliderBarImpl : public widgetBaseStruct
		{
			sliderBarComponent &data;
			skinWidgetDefaultsStruct::sliderBarStruct::directionStruct defaults;
			vec4 frame;
			elementTypeEnum baseElement;
			elementTypeEnum dotElement;

			sliderBarImpl(guiItemStruct *base) : widgetBaseStruct(base), data(GUI_REF_COMPONENT(sliderBar))
			{
				data.value = clamp(data.value, data.min, data.max);
			}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(!base->firstChild, "slider may not have children");
				CAGE_ASSERT_RUNTIME(!base->layout, "slider may not have layout");
				CAGE_ASSERT_RUNTIME(!base->text, "slider may not have text");
				CAGE_ASSERT_RUNTIME(!base->image, "slider may not have image");
			}

			virtual void updateRequestedSize() override
			{
				defaults = data.vertical ? skin().defaults.sliderBar.vertical : skin().defaults.sliderBar.horizontal;
				baseElement = data.vertical ? elementTypeEnum::SliderVerticalPanel : elementTypeEnum::SliderHorizontalPanel;
				dotElement = data.vertical ? elementTypeEnum::SliderVerticalDot : elementTypeEnum::SliderHorizontalDot;
				vec4 border = skin().layouts[(uint32)baseElement].border;
				frame = border + defaults.margin + defaults.padding;
				base->requestedSize = defaults.size;
				sizeOffset(base->requestedSize, frame);
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{
				base->contentPosition = base->position;
				base->contentSize = base->size;
				positionOffset(base->contentPosition, -frame);
				sizeOffset(base->contentSize, -frame);
			}

			virtual void emit() const override
			{
				emitElement(baseElement, 0, defaults.margin);
				real ds = min(base->contentSize[0], base->contentSize[1]);
				vec2 size = vec2(ds, ds);
				vec2 p = base->contentSize - size;
				real f = (data.value - data.min) / (data.max - data.min);
				vec2 pos = interpolate(vec2(), p, f) + base->contentPosition;
				emitElement(dotElement, 0, pos, size);
			}

			void update(vec2 point)
			{
				real ds = min(base->contentSize[0], base->contentSize[1]);
				real mp = point[data.vertical];
				real cp = base->contentPosition[data.vertical];
				real cs = base->contentSize[data.vertical];
				mp -= ds * 0.5;
				cs -= ds;
				real f = (mp - cp) / cs;
				f = clamp(f, 0, 1);
				data.value = f * (data.max - data.min) + data.min;
			}

			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				if (buttons != mouseButtonsFlags::Left)
					return true;
				if (modifiers != modifiersFlags::None)
					return true;
				update(point);
				makeFocused();
				return true;
			}

			virtual bool mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				if (hasFocus())
					return mousePress(buttons, modifiers, point);
				return false;
			}
		};
	}

	void sliderBarCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<sliderBarImpl>(item);
	}
}
