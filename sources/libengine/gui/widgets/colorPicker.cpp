#include <cage-core/color.h>

#include "../private.h"

namespace cage
{
	namespace
	{
		struct ColorPickerImpl;

		struct ColorPickerRenderable : public RenderableBase
		{
			Vec4 pos;
			Vec3 rgb;
			uint32 mode = m; // 0 = flat, 1 = hue, 2 = saturation & value

			ColorPickerRenderable(const ColorPickerImpl *item);

			~ColorPickerRenderable() override
			{
				if (!prepare())
					return;
				RenderQueue *q = impl->activeQueue;
				Holder<ShaderProgram> shader = impl->graphicsData.colorPickerShader[mode].share();
				q->bind(shader);
				q->uniform(shader, 0, pos);
				switch (mode)
				{
					case 0:
						q->uniform(shader, 1, rgb);
						break;
					case 2:
						q->uniform(shader, 1, colorRgbToHsv(rgb)[0]);
						break;
				}
				q->draw(impl->graphicsData.imageModel.share());
			}
		};

		struct ColorPickerImpl : public WidgetItem
		{
			GuiColorPickerComponent &data;
			ColorPickerImpl *small = nullptr, *large = nullptr;

			Vec3 color;
			Vec2 sliderPos, sliderSize;
			Vec2 resultPos, resultSize;
			Vec2 rectPos, rectSize;

			ColorPickerImpl(HierarchyItem *hierarchy, ColorPickerImpl *small = nullptr) : WidgetItem(hierarchy), data(GUI_REF_COMPONENT(ColorPicker)), small(small) {}

			void initialize() override
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

			void findRequestedSize() override
			{
				if (this == large)
					hierarchy->requestedSize = skin->defaults.colorPicker.fullSize;
				else if (this == small)
					hierarchy->requestedSize = skin->defaults.colorPicker.collapsedSize;
				else
					CAGE_ASSERT(false);
				offsetSize(hierarchy->requestedSize, skin->defaults.colorPicker.margin);
			}

			void findFinalPosition(const FinalPosition &update) override
			{
				if (this == large && small)
				{ // this is a popup
					hierarchy->renderSize = hierarchy->requestedSize;
					hierarchy->renderPos = small->hierarchy->renderPos + small->hierarchy->renderSize * 0.5 - hierarchy->renderSize * 0.5;
					hierarchy->moveToWindow(true, true);
				}
				if (this == large)
				{
					Vec2 p = hierarchy->renderPos;
					Vec2 s = hierarchy->renderSize;
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

			void emitColor(Vec2 pos, Vec2 size, uint32 mode, const Vec4 &margin)
			{
				ColorPickerRenderable t(this);
				offset(pos, size, -margin);
				t.setClip(hierarchy);
				t.pos = hierarchy->impl->pointsToNdc(pos, size);
				t.mode = mode;
				t.rgb = color;
			}

			void emit() override
			{
				Vec2 p = hierarchy->renderPos;
				Vec2 s = hierarchy->renderSize;
				offset(p, s, -skin->defaults.colorPicker.margin);
				if (this == large)
				{ // large
					emitElement(GuiElementTypeEnum::ColorPickerFull, mode(true, 1 | 2 | 4), p, s);
					const ElementModeEnum m = mode(false, 0);
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

			bool handleMouse(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point, bool move)
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
						Vec3 hsv = colorRgbToHsv(data.color);
						Vec2 p = clamp((point - sliderPos) / sliderSize, 0, 1);
						hsv[0] = p[0];
						data.color = colorHsvToRgb(hsv);
						hierarchy->fireWidgetEvent();
					}
					else if (hasFocus(4))
					{
						Vec3 hsv = colorRgbToHsv(data.color);
						Vec2 p = clamp((point - rectPos) / rectSize, 0, 1);
						hsv[1] = p[0];
						hsv[2] = 1 - p[1];
						data.color = colorHsvToRgb(hsv);
						hierarchy->fireWidgetEvent();
					}
				}
				return true;
			}

			bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override { return handleMouse(buttons, modifiers, point, false); }

			bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, Vec2 point) override { return handleMouse(buttons, modifiers, point, true); }
		};

		ColorPickerRenderable::ColorPickerRenderable(const ColorPickerImpl *item) : RenderableBase(item->hierarchy->impl) {}
	}

	void ColorPickerCreate(HierarchyItem *item)
	{
		CAGE_ASSERT(!item->item);
		item->item = item->impl->memory->createHolder<ColorPickerImpl>(item).cast<BaseItem>();
	}
}
