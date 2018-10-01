#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/color.h>

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
		struct colorPickerRenderableStruct : renderableBaseStruct
		{
			vec4 pos;
			vec3 rgb;
			uint32 mode; // 0 = flat, 1 = hue, 2 = saturation & value

			virtual void render(guiImpl *impl) override
			{
				shaderClass *shr = impl->graphicsData.colorPickerShader[mode];
				shr->bind();
				shr->uniform(0, pos);
				switch (mode)
				{
				case 0:
					shr->uniform(1, rgb);
					break;
				case 2:
					shr->uniform(1, convertRgbToHsv(rgb)[0]);
					break;
				}
				meshClass *mesh = impl->graphicsData.imageMesh;
				mesh->bind();
				mesh->dispatch();
			}
		};

		struct colorPickerImpl : public widgetBaseStruct
		{
			colorPickerComponent &data;
			colorPickerImpl *small, *large;

			vec2 sliderPos, sliderSize;
			vec2 resultPos, resultSize;
			vec2 rectPos, rectSize;

			colorPickerImpl(guiItemStruct *base, colorPickerImpl *small = nullptr) : widgetBaseStruct(base), data(GUI_REF_COMPONENT(colorPicker)), small(small), large(nullptr)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT_RUNTIME(!base->firstChild, "color picker may not have children");
				CAGE_ASSERT_RUNTIME(!base->layout, "color picker may not have layout");
				CAGE_ASSERT_RUNTIME(!base->text, "color picker may not have text");
				CAGE_ASSERT_RUNTIME(!base->image, "color picker may not have image");

				if (data.collapsible)
				{
					if (small)
					{ // this is the large popup
						CAGE_ASSERT_RUNTIME(small && small != this, small, this, large);
						large = this;
					}
					else
					{ // this is the small base
						if (hasFocus(1 | 2 | 4))
						{ // create popup
							guiItemStruct *item = base->impl->itemsMemory.createObject<guiItemStruct>(base->impl, base->entity);
							item->attachParent(base->impl->root);
							item->widget = large = base->impl->itemsMemory.createObject<colorPickerImpl>(item, this);
							large->widgetState = widgetState;
							small = this;
						}
						else
						{ // no popup
							small = this;
							large = nullptr;
						}
					}
				}
				else
				{ // the large base directly
					small = nullptr;
					large = this;
				}
				CAGE_ASSERT_RUNTIME(small != large);
			}

			virtual void findRequestedSize() override
			{
				if (this == large)
					base->requestedSize = skin().defaults.colorPicker.fullSize;
				else if (this == small)
					base->requestedSize = skin().defaults.colorPicker.collapsedSize;
				else
					CAGE_ASSERT_RUNTIME(false);
				offsetSize(base->requestedSize, skin().defaults.colorPicker.margin);
			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{
				if (this == large && small)
				{ // this is a popup
					base->size = base->requestedSize;
					base->position = small->base->position + small->base->size * 0.5 - base->size * 0.5;
					base->moveToWindow(true, true);
				}
				if (this == large)
				{
					vec2 p = base->position;
					vec2 s = base->size;
					offset(p, s, -skin().defaults.colorPicker.margin - skin().layouts[(uint32)elementTypeEnum::ColorPickerFull].border);
					sliderPos = p;
					sliderSize = s;
					sliderSize[1] *= skin().defaults.colorPicker.hueBarPortion;
					resultPos = p;
					resultPos[1] += sliderSize[1];
					resultSize = s;
					resultSize[0] *= skin().defaults.colorPicker.resultBarPortion;
					resultSize[1] *= 1 - skin().defaults.colorPicker.hueBarPortion;
					rectPos = resultPos;
					rectPos[0] += resultSize[0];
					rectSize[0] = s[0] * (1 - skin().defaults.colorPicker.resultBarPortion);
					rectSize[1] = resultSize[1];
				}
			}

			void emitColor(vec2 pos, vec2 size, uint32 mode, const vec4 &margin) const
			{
				auto *e = base->impl->emitControl;
				auto *t = e->memory.createObject<colorPickerRenderableStruct>();
				offset(pos, size, -margin);
				t->pos = base->impl->pointsToNdc(pos, size);
				t->mode = mode;
				t->rgb = data.color;
				e->last->next = t;
				e->last = t;
			}

			virtual void emit() const override
			{
				vec2 p = base->position;
				vec2 s = base->size;
				offset(p, s, -skin().defaults.colorPicker.margin);
				if (this == large)
				{ // large
					emitElement(elementTypeEnum::ColorPickerFull, mode(true, 1 | 2 | 4), p, s);
					uint32 m = mode(false, 0);
					emitElement(elementTypeEnum::ColorPickerHSliderPanel, m, sliderPos, sliderSize);
					emitElement(elementTypeEnum::ColorPickerSVRect, m, rectPos, rectSize);
					emitElement(elementTypeEnum::ColorPickerResult, m, resultPos, resultSize);
					emitColor(sliderPos, sliderSize, 1, skin().layouts[(uint32)elementTypeEnum::ColorPickerHSliderPanel].border);
					emitColor(rectPos, rectSize, 2, skin().layouts[(uint32)elementTypeEnum::ColorPickerSVRect].border);
					emitColor(resultPos, resultSize, 0, skin().layouts[(uint32)elementTypeEnum::ColorPickerResult].border);
				}
				else
				{ // small
					emitElement(elementTypeEnum::ColorPickerCompact, mode(true, 1 | 2 | 4), p, s);
					emitColor(p, s, 0, skin().layouts[(uint32)elementTypeEnum::ColorPickerCompact].border);
				}
			}

			bool handleMouse(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point, bool move)
			{
				if (!move)
					makeFocused();
				if (buttons != mouseButtonsFlags::Left)
					return true;
				if (modifiers != modifiersFlags::None)
					return true;
				if (this == large)
				{
					if (!move)
					{
						if (pointInside(sliderPos, sliderSize, point))
							makeFocused(2);
						else if (pointInside(rectPos, rectSize, point))
							makeFocused(4);
					}
					if (hasFocus(2))
					{
						vec3 hsv = convertRgbToHsv(data.color);
						vec2 p = clamp((point - sliderPos) / sliderSize, 0, 1);
						hsv[0] = p[0];
						data.color = convertHsvToRgb(hsv);
						base->impl->widgetEvent.dispatch(base->entity->name());
					}
					else if (hasFocus(4))
					{
						vec3 hsv = convertRgbToHsv(data.color);
						vec2 p = clamp((point - rectPos) / rectSize, 0, 1);
						hsv[1] = p[0];
						hsv[2] = 1 - p[1];
						data.color = convertHsvToRgb(hsv);
						base->impl->widgetEvent.dispatch(base->entity->name());
					}
				}
				return true;
			}

			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				return handleMouse(buttons, modifiers, point, false);
			}

			virtual bool mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				return handleMouse(buttons, modifiers, point, true);
			}
		};
	}

	void colorPickerCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<colorPickerImpl>(item);
	}
}
