#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/utility/color.h>

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
		struct colorPickerRenderableStruct : renderableBaseStruct
		{
			vec4 pos;
			vec3 rgb;
			uint32 mode; // 0 = flat, 1 = hue, 2 = saturation & value

			virtual void render(guiImpl *impl) override
			{
				shaderClass *shr = impl->graphicData.colorPickerShader[mode];
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
				meshClass *mesh = impl->graphicData.imageMesh;
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
						if (hasFocus())
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

			virtual void updateRequestedSize() override
			{
				if (this == large)
					base->requestedSize = skin().defaults.colorPicker.fullSize;
				else if (this == small)
					base->requestedSize = skin().defaults.colorPicker.collapsedSize;
				else
					CAGE_ASSERT_RUNTIME(false);
			}

			virtual void updateFinalPosition(const updatePositionStruct &update) override
			{
				vec4 border;
				if (this == large && small)
				{ // this is a popup
					base->size = base->requestedSize;
					base->position = small->base->position + small->base->size * 0.5 - base->size * 0.5;
					base->contentSize = base->size;
					base->contentPosition = base->position;
					border = skin().layouts[(uint32)elementTypeEnum::ColorPickerFull].border;
				}
				else
				{
					border = skin().layouts[(uint32)elementTypeEnum::ColorPickerCompact].border;
				}
				base->contentSize = base->size;
				base->contentPosition = base->position;
				positionOffset(base->contentPosition, -border);
				sizeOffset(base->contentSize, -border);
				if (this == large && small)
					base->moveToWindow(true, true);
				if (this == large)
				{
					sliderPos = base->contentPosition;
					sliderSize = base->contentSize;
					sliderSize[1] *= skin().defaults.colorPicker.hueBarPortion;
					resultPos = base->contentPosition;
					resultPos[1] += sliderSize[1];
					resultSize = base->contentSize;
					resultSize[0] *= skin().defaults.colorPicker.resultBarPortion;
					resultSize[1] *= 1 - skin().defaults.colorPicker.hueBarPortion;
					rectPos = resultPos;
					rectPos[0] += resultSize[0];
					rectSize[0] = base->contentSize[0] * (1 - skin().defaults.colorPicker.resultBarPortion);
					rectSize[1] = resultSize[1];
				}
			}

			void emitColor(vec2 pos, vec2 size, uint32 mode, const vec4 &margin = vec4()) const
			{
				auto *e = base->impl->emitControl;
				auto *t = e->memory.createObject<colorPickerRenderableStruct>();
				positionOffset(pos, -margin);
				sizeOffset(size, -margin);
				t->pos = base->impl->pointsToNdc(pos, size);
				t->mode = mode;
				t->rgb = data.color;
				e->last->next = t;
				e->last = t;
			}

			virtual void emit() const override
			{
				if (this == large)
				{ // large
					emitElement(elementTypeEnum::ColorPickerFull, 0, vec4());
					emitElement(elementTypeEnum::ColorPickerHSliderPanel, 0, sliderPos, sliderSize);
					emitElement(elementTypeEnum::ColorPickerResult, 0, resultPos, resultSize);
					emitElement(elementTypeEnum::ColorPickerSVRect, 0, rectPos, rectSize);
					emitColor(sliderPos, sliderSize, 1, skin().layouts[(uint32)elementTypeEnum::ColorPickerHSliderPanel].border);
					emitColor(resultPos, resultSize, 0, skin().layouts[(uint32)elementTypeEnum::ColorPickerResult].border);
					emitColor(rectPos, rectSize, 2, skin().layouts[(uint32)elementTypeEnum::ColorPickerSVRect].border);
				}
				else
				{ // small
					emitElement(elementTypeEnum::ColorPickerCompact, 0, vec4());
					emitColor(base->contentPosition, base->contentSize, 0);
				}
			}

			bool check(vec2 pos, vec2 size, vec2 &point)
			{
				vec2 p = (point - pos) / size;
				if (p[0] >= 0 && p[0] <= 1 && p[1] >= 0 && p[1] <= 1)
				{
					point = p;
					return true;
				}
				return false;
			}

			void update(vec2 point)
			{
				if (check(sliderPos, sliderSize, point))
				{
					vec3 hsv = convertRgbToHsv(data.color);
					hsv[0] = point[0];
					data.color = convertHsvToRgb(hsv);
				}
				else if (check(rectPos, rectSize, point))
				{
					vec3 hsv = convertRgbToHsv(data.color);
					hsv[1] = point[0];
					hsv[2] = 1 - point[1];
					data.color = convertHsvToRgb(hsv);
				}
			}

			virtual bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, vec2 point) override
			{
				if (buttons != mouseButtonsFlags::Left)
					return true;
				if (modifiers != modifiersFlags::None)
					return true;
				if (this == large)
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

	void colorPickerCreate(guiItemStruct *item)
	{
		item->widget = item->impl->itemsMemory.createObject<colorPickerImpl>(item);
	}
}
