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
	namespace
	{
		struct sliderBarImpl : public widgetBaseStruct
		{
			sliderBarComponent &data;
			skinWidgetDefaultsStruct::sliderBarStruct::directionStruct defaults;
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
				base->requestedSize = defaults.size;
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{
				base->updateContentPosition(defaults.margin);
			}

			virtual void emit() const override
			{
				CAGE_ASSERT_RUNTIME(data.value.valid() && data.min.valid() && data.max.valid());
				CAGE_ASSERT_RUNTIME(data.max > data.min);
				emitElement(baseElement, mode(false));
				real ds = min(base->contentSize[0], base->contentSize[1]);
				vec2 size = vec2(ds, ds);
				vec2 p = base->contentSize - size;
				real f = (data.value - data.min) / (data.max - data.min);
				vec2 pos = vec2(interpolate(0, p[0], f), interpolate(p[1], 0, f)) + base->contentPosition;
				offset(pos, size, -skin().layouts[(uint32)baseElement].border);
				emitElement(dotElement, mode(), pos, size);
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
				if (data.vertical)
					f = 1 - f;
				data.value = f * (data.max - data.min) + data.min;
				base->impl->widgetEvent.dispatch(base->entity->getName());
			}

			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				makeFocused();
				if (buttons != mouseButtonsFlags::Left)
					return true;
				if (modifiers != modifiersFlags::None)
					return true;
				update(point);
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
