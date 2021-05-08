#include <cage-core/color.h>

#include "../private.h"

namespace cage
{
	namespace
	{
		struct ColorPickerImpl;

		struct ColorPickerRenderable : public RenderableBase
		{
			vec4 pos;
			vec3 rgb;
			uint32 mode = m; // 0 = flat, 1 = hue, 2 = saturation & value

			ColorPickerRenderable(const ColorPickerImpl *item);

			virtual ~ColorPickerRenderable() override
			{
				if (!prepare())
					return;
				RenderQueue *q = impl->activeQueue;
				q->bind(impl->graphicsData.colorPickerShader[mode].share());
				q->uniform(0, pos);
				switch (mode)
				{
				case 0:
					q->uniform(1, rgb);
					break;
				case 2:
					q->uniform(1, colorRgbToHsv(rgb)[0]);
					break;
				}
				q->bind(impl->graphicsData.imageModel.share());
				q->draw();
			}
		};

		struct ColorPickerImpl : public WidgetItem
		{
			GuiColorPickerComponent &data;
			ColorPickerImpl *small = nullptr, *large = nullptr;

			vec3 color;
			vec2 sliderPos, sliderSize;
			vec2 resultPos, resultSize;
			vec2 rectPos, rectSize;

			ColorPickerImpl(HierarchyItem *hierarchy, ColorPickerImpl *small = nullptr) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(ColorPicker)), small(small)
			{}

			virtual void initialize() override
			{
				CAGE_ASSERT(hierarchy->children.empty());
				CAGE_ASSERT(!hierarchy->text);
				CAGE_ASSERT(!hierarchy->image);

				if (data.collapsible)
				{
					if (small)
					{ // this is the large popup
						CAGE_ASSERT(small && small != this);
						large = this;
					}
					else
					{ // this is the small base
						if (hasFocus(1 | 2 | 4))
						{ // create popup
							Holder<HierarchyItem> item = hierarchy->impl->memory->createHolder<HierarchyItem>(hierarchy->impl, hierarchy->ent);
							item->item = hierarchy->impl->memory->createHolder<ColorPickerImpl>(+item, this).cast<BaseItem>();
							large = class_cast<ColorPickerImpl *>(+item->item);
							large->widgetState = widgetState;
							large->skin = skin;
							small = this;
							hierarchy->impl->root->children.push_back(std::move(item));
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

			virtual void findFinalPosition(const FinalPosition &update) override
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
					offset(p, s, -skin->defaults.colorPicker.margin - skin->layouts[(uint32)GuiElementTypeEnum::ColorPickerFull].border);
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

			void emitColor(vec2 pos, vec2 size, uint32 mode, const vec4 &margin)
			{
				ColorPickerRenderable t(this);
				offset(pos, size, -margin);
				t.setClip(hierarchy);
				t.pos = hierarchy->impl->pointsToNdc(pos, size);
				t.mode = mode;
				t.rgb = color;
			}

			virtual void emit() override
			{
				vec2 p = hierarchy->renderPos;
				vec2 s = hierarchy->renderSize;
				offset(p, s, -skin->defaults.colorPicker.margin);
				if (this == large)
				{ // large
					emitElement(GuiElementTypeEnum::ColorPickerFull, mode(true, 1 | 2 | 4), p, s);
					uint32 m = mode(false, 0);
					emitElement(GuiElementTypeEnum::ColorPickerHuePanel, m, sliderPos, sliderSize);
					emitElement(GuiElementTypeEnum::ColorPickerSatValPanel, m, rectPos, rectSize);
					emitElement(GuiElementTypeEnum::ColorPickerPreviewPanel, m, resultPos, resultSize);
					emitColor(sliderPos, sliderSize, 1, skin->layouts[(uint32)GuiElementTypeEnum::ColorPickerHuePanel].border);
					emitColor(rectPos, rectSize, 2, skin->layouts[(uint32)GuiElementTypeEnum::ColorPickerSatValPanel].border);
					emitColor(resultPos, resultSize, 0, skin->layouts[(uint32)GuiElementTypeEnum::ColorPickerPreviewPanel].border);
				}
				else
				{ // small
					emitElement(GuiElementTypeEnum::ColorPickerCompact, mode(true, 1 | 2 | 4), p, s);
					emitColor(p, s, 0, skin->layouts[(uint32)GuiElementTypeEnum::ColorPickerCompact].border);
				}
			}

			bool handleMouse(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point, bool move)
			{
				if (!move)
					makeFocused();
				if (buttons != MouseButtonsFlags::Left)
					return true;
				if (modifiers != ModifiersFlags::None)
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
						vec3 hsv = colorRgbToHsv(data.color);
						vec2 p = clamp((point - sliderPos) / sliderSize, 0, 1);
						hsv[0] = p[0];
						data.color = colorHsvToRgb(hsv);
						hierarchy->fireWidgetEvent();
					}
					else if (hasFocus(4))
					{
						vec3 hsv = colorRgbToHsv(data.color);
						vec2 p = clamp((point - rectPos) / rectSize, 0, 1);
						hsv[1] = p[0];
						hsv[2] = 1 - p[1];
						data.color = colorHsvToRgb(hsv);
						hierarchy->fireWidgetEvent();
					}
				}
				return true;
			}

			virtual bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override
			{
				return handleMouse(buttons, modifiers, point, false);
			}

			virtual bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, vec2 point) override
			{
				return handleMouse(buttons, modifiers, point, true);
			}
		};

		ColorPickerRenderable::ColorPickerRenderable(const ColorPickerImpl *item) : RenderableBase(item->hierarchy->impl)
		{}
	}

	void ColorPickerCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<ColorPickerImpl>(item).cast<BaseItem>();
	}
}
