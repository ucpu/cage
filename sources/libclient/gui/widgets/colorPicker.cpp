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
				shaderProgram *shr = impl->graphicsData.colorPickerShader[mode];
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
				renderMesh *mesh = impl->graphicsData.imageMesh;
				mesh->bind();
				mesh->dispatch();
			}
		};

		struct colorPickerImpl : public widgetItemStruct
		{
			colorPickerComponent &data;
			colorPickerImpl *small, *large;

			vec3 color;
			vec2 sliderPos, sliderSize;
			vec2 resultPos, resultSize;
			vec2 rectPos, rectSize;

			colorPickerImpl(hierarchyItemStruct *hierarchy, colorPickerImpl *small = nullptr) : widgetItemStruct(hierarchy), data(GUI_REF_COMPONENT(colorPicker)), small(small), large(nullptr)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT(!hierarchy->firstChild, "color picker may not have children");
				CAGE_ASSERT(!hierarchy->text, "color picker may not have text");
				CAGE_ASSERT(!hierarchy->image, "color picker may not have image");

				if (data.collapsible)
				{
					if (small)
					{ // this is the large popup
						CAGE_ASSERT(small && small != this, small, this, large);
						large = this;
					}
					else
					{ // this is the small base
						if (hasFocus(1 | 2 | 4))
						{ // create popup
							hierarchyItemStruct *item = hierarchy->impl->itemsMemory.createObject<hierarchyItemStruct>(hierarchy->impl, hierarchy->ent);
							item->attachParent(hierarchy->impl->root);
							item->item = large = hierarchy->impl->itemsMemory.createObject<colorPickerImpl>(item, this);
							large->widgetState = widgetState;
							large->skin = skin;
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
				CAGE_ASSERT(small != large);
				color = data.color;
			}

			virtual void findRequestedSize() override
			{
				if (this == large)
					hierarchy->requestedSize = skin->defaults.colorPicker.fullSize;
				else if (this == small)
					hierarchy->requestedSize = skin->defaults.colorPicker.collapsedSize;
				else
					CAGE_ASSERT(false);
				offsetSize(hierarchy->requestedSize, skin->defaults.colorPicker.margin);
			}

			virtual void findFinalPosition(const finalPositionStruct &update) override
			{
				if (this == large && small)
				{ // this is a popup
					hierarchy->renderSize = hierarchy->requestedSize;
					hierarchy->renderPos = small->hierarchy->renderPos + small->hierarchy->renderSize * 0.5 - hierarchy->renderSize * 0.5;
					hierarchy->moveToWindow(true, true);
				}
				if (this == large)
				{
					vec2 p = hierarchy->renderPos;
					vec2 s = hierarchy->renderSize;
					offset(p, s, -skin->defaults.colorPicker.margin - skin->layouts[(uint32)elementTypeEnum::ColorPickerFull].border);
					sliderPos = p;
					sliderSize = s;
					sliderSize[1] *= skin->defaults.colorPicker.hueBarPortion;
					resultPos = p;
					resultPos[1] += sliderSize[1];
					resultSize = s;
					resultSize[0] *= skin->defaults.colorPicker.resultBarPortion;
					resultSize[1] *= 1 - skin->defaults.colorPicker.hueBarPortion;
					rectPos = resultPos;
					rectPos[0] += resultSize[0];
					rectSize[0] = s[0] * (1 - skin->defaults.colorPicker.resultBarPortion);
					rectSize[1] = resultSize[1];
				}
			}

			void emitColor(vec2 pos, vec2 size, uint32 mode, const vec4 &margin) const
			{
				auto *e = hierarchy->impl->emitControl;
				auto *t = e->memory.createObject<colorPickerRenderableStruct>();
				offset(pos, size, -margin);
				t->setClip(hierarchy);
				t->pos = hierarchy->impl->pointsToNdc(pos, size);
				t->mode = mode;
				t->rgb = color;
				e->last->next = t;
				e->last = t;
			}

			virtual void emit() const override
			{
				vec2 p = hierarchy->renderPos;
				vec2 s = hierarchy->renderSize;
				offset(p, s, -skin->defaults.colorPicker.margin);
				if (this == large)
				{ // large
					emitElement(elementTypeEnum::ColorPickerFull, mode(true, 1 | 2 | 4), p, s);
					uint32 m = mode(false, 0);
					emitElement(elementTypeEnum::ColorPickerHuePanel, m, sliderPos, sliderSize);
					emitElement(elementTypeEnum::ColorPickerSatValPanel, m, rectPos, rectSize);
					emitElement(elementTypeEnum::ColorPickerPreviewPanel, m, resultPos, resultSize);
					emitColor(sliderPos, sliderSize, 1, skin->layouts[(uint32)elementTypeEnum::ColorPickerHuePanel].border);
					emitColor(rectPos, rectSize, 2, skin->layouts[(uint32)elementTypeEnum::ColorPickerSatValPanel].border);
					emitColor(resultPos, resultSize, 0, skin->layouts[(uint32)elementTypeEnum::ColorPickerPreviewPanel].border);
				}
				else
				{ // small
					emitElement(elementTypeEnum::ColorPickerCompact, mode(true, 1 | 2 | 4), p, s);
					emitColor(p, s, 0, skin->layouts[(uint32)elementTypeEnum::ColorPickerCompact].border);
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
						hierarchy->fireWidgetEvent();
					}
					else if (hasFocus(4))
					{
						vec3 hsv = convertRgbToHsv(data.color);
						vec2 p = clamp((point - rectPos) / rectSize, 0, 1);
						hsv[1] = p[0];
						hsv[2] = 1 - p[1];
						data.color = convertHsvToRgb(hsv);
						hierarchy->fireWidgetEvent();
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

	void colorPickerCreate(hierarchyItemStruct *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->itemsMemory.createObject<colorPickerImpl>(item);
	}
}
