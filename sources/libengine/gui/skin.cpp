#include "private.h"

#include <cage-core/flatSet.h>
#include <cage-core/hashString.h>

namespace cage
{
	namespace
	{
		consteval GuiTextFormatComponent textFormatComponentInit()
		{
			GuiTextFormatComponent text{ .font = 0 };
			text.color = Vec3(1);
			text.font = 0;
			text.size = 14;
			text.align = TextAlignEnum::Left;
			text.lineSpacing = 1;
			return text;
		}

		consteval GuiImageFormatComponent imageFormatComponentInit()
		{
			GuiImageFormatComponent Image;
			Image.animationOffset = 0;
			Image.animationSpeed = 1;
			Image.mode = ImageModeEnum::Stretch;
			return Image;
		}

		constexpr GuiTextFormatComponent TextInit = textFormatComponentInit();
		constexpr GuiImageFormatComponent ImageInit = imageFormatComponentInit();

		constexpr uint32 ClickSound = HashString("cage/sounds/click.wav");
		constexpr uint32 SlidingSound = HashString("cage/sounds/sliding.wav");
	}

	namespace
	{
		struct Packer
		{
			Vec2 size; // including border
			Real frame; // spacing around element
			Real border; // border inside element

			constexpr GuiSkinElementLayout::TextureUvOi next(bool allowBorder)
			{
				Vec2 s = size + frame * 2;
				CAGE_ASSERT(s[0] < 1);
				if (p[0] + s[0] > 1)
					newLine();
				GuiSkinElementLayout::TextureUvOi r;
				r.outer[0] = p[0] + frame;
				r.outer[1] = p[1] + frame;
				r.outer[2] = r.outer[0] + size[0];
				r.outer[3] = r.outer[1] + size[1];
				r.inner = r.outer;
				if (allowBorder)
				{
					CAGE_ASSERT(size[0] >= 2 * border && size[1] >= 2 * border);
					r.inner[0] += border;
					r.inner[1] += border;
					r.inner[2] -= border;
					r.inner[3] -= border;
				}
				my = max(my, p[1] + s[1]);
				p[0] += s[0];
				return r;
			}

			constexpr void newLine()
			{
				p[0] = 0;
				p[1] = my;
			}

		private:
			Vec2 p;
			Real my;
		};

		struct LayoutsBase
		{
			GuiSkinElementLayout layouts[(uint32)GuiElementTypeEnum::TotalElements];

			consteval LayoutsBase()
			{
				std::vector<GuiElementTypeEnum> largeElements = {
					GuiElementTypeEnum::PanelBase /* overlaps: SpoilerBase */,
					GuiElementTypeEnum::TextArea,
				};

				std::vector<GuiElementTypeEnum> smallElements = {
					GuiElementTypeEnum::ScrollbarHorizontalPanel,
					GuiElementTypeEnum::ScrollbarVerticalPanel,
					GuiElementTypeEnum::ScrollbarHorizontalDot,
					GuiElementTypeEnum::ScrollbarVerticalDot,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::UnusedElement,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::PanelCaption,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::SpoilerCaption,
					GuiElementTypeEnum::SpoilerIconCollapsed,
					GuiElementTypeEnum::SpoilerIconShown,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::Button,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::Input,
					GuiElementTypeEnum::InputButtonDecrement,
					GuiElementTypeEnum::InputButtonIncrement,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::CheckBoxUnchecked,
					GuiElementTypeEnum::CheckBoxChecked,
					GuiElementTypeEnum::CheckBoxIndetermined,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::RadioBoxUnchecked,
					GuiElementTypeEnum::RadioBoxChecked,
					GuiElementTypeEnum::RadioBoxIndetermined,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::UnusedElement,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::ComboBoxBase,
					GuiElementTypeEnum::ComboBoxList,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::ComboBoxItemUnchecked,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::ComboBoxItemChecked,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::SliderHorizontalPanel,
					GuiElementTypeEnum::SliderVerticalPanel,
					GuiElementTypeEnum::SliderHorizontalDot,
					GuiElementTypeEnum::SliderVerticalDot,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::ProgressBar,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::ColorPickerCompact /* overlaps: ColorPickerFull */,
					GuiElementTypeEnum::ColorPickerPreviewPanel /* overlaps: ColorPickerHuePanel, ColorPickerSatValPanel */,
				};

				FlatSet<GuiElementTypeEnum> wideElements = {
					GuiElementTypeEnum::PanelCaption,
					GuiElementTypeEnum::SpoilerCaption,
					GuiElementTypeEnum::Button,
					GuiElementTypeEnum::Input,
					GuiElementTypeEnum::ComboBoxBase,
					GuiElementTypeEnum::ComboBoxList,
					GuiElementTypeEnum::ComboBoxItemUnchecked,
					GuiElementTypeEnum::ComboBoxItemChecked,
					GuiElementTypeEnum::ProgressBar,
				};

				FlatSet<GuiElementTypeEnum> noBorder = {
					GuiElementTypeEnum::SpoilerIconCollapsed,
					GuiElementTypeEnum::SpoilerIconShown,
					GuiElementTypeEnum::RadioBoxChecked,
					GuiElementTypeEnum::RadioBoxUnchecked,
					GuiElementTypeEnum::RadioBoxIndetermined,
					GuiElementTypeEnum::SliderHorizontalDot,
					GuiElementTypeEnum::SliderVerticalDot,
				};

				FlatSet<GuiElementTypeEnum> noFocus = {
					GuiElementTypeEnum::PanelBase,
					GuiElementTypeEnum::PanelCaption,
					GuiElementTypeEnum::SpoilerBase,
					GuiElementTypeEnum::SpoilerCaption,
					GuiElementTypeEnum::SpoilerIconCollapsed,
					GuiElementTypeEnum::SpoilerIconShown,
					GuiElementTypeEnum::InputButtonDecrement,
					GuiElementTypeEnum::InputButtonIncrement,
					GuiElementTypeEnum::ComboBoxList,
					GuiElementTypeEnum::ComboBoxItemUnchecked,
					GuiElementTypeEnum::ComboBoxItemChecked,
					GuiElementTypeEnum::ProgressBar,
					GuiElementTypeEnum::ColorPickerHuePanel,
					GuiElementTypeEnum::ColorPickerPreviewPanel,
					GuiElementTypeEnum::ColorPickerSatValPanel,
				};

				FlatSet<GuiElementTypeEnum> noHover = {
					GuiElementTypeEnum::PanelBase,
					GuiElementTypeEnum::SpoilerBase,
					GuiElementTypeEnum::SpoilerIconCollapsed,
					GuiElementTypeEnum::SpoilerIconShown,
					GuiElementTypeEnum::ComboBoxList,
					GuiElementTypeEnum::ColorPickerHuePanel,
					GuiElementTypeEnum::ColorPickerPreviewPanel,
					GuiElementTypeEnum::ColorPickerSatValPanel,
				};

				{ // automatic uv construction
					Packer packer;
					packer.frame = 4.f / 1024;
					packer.border = 4.f / 1024;

					// large
					packer.size = Vec2(5, 5) * 24.f / 1024;
					for (auto it : largeElements)
					{
						if (it == GuiElementTypeEnum::InvalidElement)
						{
							packer.newLine();
							continue;
						}
						if (it >= GuiElementTypeEnum::UnusedElement)
						{
							packer.next(false);
							continue;
						}
						bool border = noBorder.count(it) == 0;
						layouts[(uint32)it].textureUv.data[0] = packer.next(border);
						if (noFocus.count(it) == 0)
							layouts[(uint32)it].textureUv.data[1] = packer.next(border);
						if (noHover.count(it) == 0)
							layouts[(uint32)it].textureUv.data[2] = packer.next(border);
						layouts[(uint32)it].textureUv.data[3] = packer.next(border);
					}
					packer.newLine();

					// small / wide
					for (auto it : smallElements)
					{
						if (it == GuiElementTypeEnum::InvalidElement)
						{
							packer.newLine();
							continue;
						}
						if (it >= GuiElementTypeEnum::UnusedElement)
						{
							packer.next(false);
							continue;
						}
						if (wideElements.count(it))
							packer.size = Vec2(5, 1) * 24.f / 1024;
						else
							packer.size = Vec2(1, 1) * 24.f / 1024;
						bool border = noBorder.count(it) == 0;
						layouts[(uint32)it].textureUv.data[0] = packer.next(border);
						if (noFocus.count(it) == 0)
							layouts[(uint32)it].textureUv.data[1] = packer.next(border);
						if (noHover.count(it) == 0)
							layouts[(uint32)it].textureUv.data[2] = packer.next(border);
						layouts[(uint32)it].textureUv.data[3] = packer.next(border);
					}
					packer.newLine();
				}

				{ // no border
					for (auto it : noBorder)
						layouts[(uint32)it].border = Vec4();
				}

				{ // overlaps
					layouts[(uint32)GuiElementTypeEnum::ColorPickerFull].textureUv = layouts[(uint32)GuiElementTypeEnum::ColorPickerCompact].textureUv;
					layouts[(uint32)GuiElementTypeEnum::ColorPickerHuePanel].textureUv = layouts[(uint32)GuiElementTypeEnum::ColorPickerPreviewPanel].textureUv;
					layouts[(uint32)GuiElementTypeEnum::ColorPickerSatValPanel].textureUv = layouts[(uint32)GuiElementTypeEnum::ColorPickerPreviewPanel].textureUv;
					layouts[(uint32)GuiElementTypeEnum::SpoilerBase].textureUv = layouts[(uint32)GuiElementTypeEnum::PanelBase].textureUv;
				}
			}
		};

		constexpr const LayoutsBase layoutsBase;
	}

	GuiSkinConfig::GuiSkinConfig() : textureName(HashString("cage/textures/gui.png")), hoverSound(HashString("cage/sounds/hover.wav"))
	{
		std::copy(std::begin(layoutsBase.layouts), std::end(layoutsBase.layouts), std::begin(layouts));
	}

	GuiSkinWidgetDefaults::Label::Label() : textFormat(TextInit), imageFormat(ImageInit)
	{
		textFormat.size = 16;
	}

	GuiSkinWidgetDefaults::Button::Button() : textFormat(TextInit), imageFormat(ImageInit), clickSound(ClickSound)
	{
		textFormat.align = TextAlignEnum::Center;
	}

	GuiSkinWidgetDefaults::Input::Input() : textValidFormat(TextInit), textInvalidFormat(TextInit), placeholderFormat(TextInit)
	{
		textInvalidFormat.color = Vec3(1, 0, 0);
		placeholderFormat.color = Vec3(0.5);
		typingValidSound = HashString("cage/sounds/typing.wav");
		typingInvalidSound = HashString("cage/sounds/typingInvalid.wav");
		confirmationSound = HashString("cage/sounds/typingConfirm.wav");
	}

	GuiSkinWidgetDefaults::TextArea::TextArea() : textFormat(TextInit) {}

	GuiSkinWidgetDefaults::CheckBox::CheckBox() : textFormat(TextInit), clickSound(ClickSound)
	{
		textFormat.size = GuiSkinWidgetDefaults::Label().textFormat.size;
	}

	GuiSkinWidgetDefaults::RadioBox::RadioBox() : textFormat(TextInit), clickSound(ClickSound)
	{
		textFormat.size = GuiSkinWidgetDefaults::Label().textFormat.size;
	}

	GuiSkinWidgetDefaults::ComboBox::ComboBox() : placeholderFormat(TextInit), itemsFormat(TextInit), selectedFormat(TextInit)
	{
		placeholderFormat.color = Vec3(0.5);
		placeholderFormat.align = TextAlignEnum::Center;
		itemsFormat.align = TextAlignEnum::Center;
		selectedFormat.align = TextAlignEnum::Center;
		openSound = HashString("cage/sounds/comboOpen.wav");
		selectSound = HashString("cage/sounds/comboSelect.wav");
	}

	GuiSkinWidgetDefaults::ProgressBar::ProgressBar() : textFormat(TextInit)
	{
		textFormat.align = TextAlignEnum::Center;
		fillingTextureName = HashString("cage/textures/progressbar/$.png");
	}

	GuiSkinWidgetDefaults::SliderBar::SliderBar() : slidingSound(SlidingSound)
	{
		horizontal.size = Vec2(150, 28);
		vertical.size = Vec2(28, 150);
	}

	GuiSkinWidgetDefaults::ColorPicker::ColorPicker() : slidingSound(SlidingSound) {}

	GuiSkinWidgetDefaults::Panel::Panel() : textFormat(TextInit), imageFormat(ImageInit)
	{
		textFormat.align = TextAlignEnum::Center;
	}

	GuiSkinWidgetDefaults::Spoiler::Spoiler() : textFormat(TextInit), imageFormat(ImageInit), clickSound(ClickSound)
	{
		textFormat.align = TextAlignEnum::Center;
	}

	GuiSkinWidgetDefaults::Scrollbars::Scrollbars() : slidingSound(SlidingSound) {}

	namespace
	{
		void generateLarge(GuiSkinConfig &skin)
		{
			GuiSkinWidgetDefaults &d = skin.defaults;

			d.label.margin = Vec4(6);
			d.label.textFormat.size = 20;
			d.label.textFormat.lineSpacing = 1.1;

			d.button.padding = Vec4(3);
			d.button.margin = Vec4(2);
			d.button.size[1] = 34;
			d.button.textFormat.size = 18;

			d.inputBox.basePadding = Vec4(3);
			d.inputBox.margin = Vec4(2);
			d.inputBox.size[1] = 34;
			d.inputBox.buttonsWidth = 34;
			d.inputBox.textValidFormat.size = d.inputBox.textInvalidFormat.size = d.inputBox.placeholderFormat.size = 18;

			d.textArea.margin = Vec4(2);
			d.textArea.padding = Vec4(3);
			d.textArea.size *= 34.0 / 28.0;
			d.textArea.textFormat.size = 18;

			d.checkBox.margin = Vec4(2);
			d.checkBox.size = Vec2(34);
			d.checkBox.textFormat.size = 18;
			d.checkBox.labelOffset = Vec2(5, 8);

			d.radioBox.margin = Vec4(2);
			d.radioBox.size = Vec2(34);
			d.radioBox.textFormat.size = 18;
			d.radioBox.labelOffset = Vec2(5, 7);

			d.comboBox.basePadding = Vec4(3);
			d.comboBox.baseMargin = Vec4(2);
			d.comboBox.listPadding = Vec4(-5);
			d.comboBox.itemPadding = Vec4(2);
			d.comboBox.size[1] = 34;
			d.comboBox.listOffset = -2;
			d.comboBox.itemSpacing = -3;
			d.comboBox.itemsFormat.size = d.comboBox.selectedFormat.size = d.comboBox.placeholderFormat.size = 18;

			d.progressBar.baseMargin = Vec4(2);
			d.progressBar.textPadding = Vec4(3);
			d.progressBar.size[1] = 34;
			d.progressBar.textFormat.size = 18;

			d.sliderBar.horizontal.padding = Vec4(2);
			d.sliderBar.vertical.padding = Vec4(2);
			d.sliderBar.horizontal.margin = Vec4(2);
			d.sliderBar.vertical.margin = Vec4(2);
			d.sliderBar.horizontal.size[1] = 34;
			d.sliderBar.vertical.size[0] = 34;

			d.colorPicker.margin = Vec4(2);
			d.colorPicker.collapsedSize = Vec2(34);
			d.colorPicker.fullSize *= 34.0 / 28.0;

			d.panel.baseMargin = Vec4(2);
			d.panel.contentPadding = Vec4(3);
			d.panel.captionPadding = Vec4(3);
			d.panel.captionHeight = 36;
			d.panel.textFormat.size = 18;

			d.spoiler.baseMargin = Vec4(2);
			d.spoiler.contentPadding = Vec4(3);
			d.spoiler.captionPadding = Vec4(3);
			d.spoiler.captionHeight = 36;
			d.spoiler.textFormat.size = 18;
		}

		void generateCompact(GuiSkinConfig &skin)
		{
			GuiSkinWidgetDefaults &d = skin.defaults;

			d.label.margin = Vec4(2);
			d.label.textFormat.size = 13;
			d.label.textFormat.lineSpacing = 0.9;

			d.button.padding = Vec4();
			d.button.size[1] = 20;
			d.button.textFormat.size = 11;

			d.inputBox.basePadding = Vec4();
			d.inputBox.size[1] = 20;
			d.inputBox.buttonsWidth = 20;
			d.inputBox.textValidFormat.size = d.inputBox.textInvalidFormat.size = d.inputBox.placeholderFormat.size = 11;
			d.inputBox.buttonsOffset = 1;

			d.textArea.padding = Vec4(1);
			d.textArea.size *= 20.0 / 28.0;
			d.textArea.textFormat.size = 11;

			d.checkBox.size = Vec2(20);
			d.checkBox.textFormat.size = 11;
			d.checkBox.labelOffset = Vec2(2, 5);

			d.radioBox.size = Vec2(20);
			d.radioBox.textFormat.size = 11;
			d.radioBox.labelOffset = Vec2(2, 4);

			d.comboBox.basePadding = Vec4();
			d.comboBox.listPadding = Vec4(-4);
			d.comboBox.itemPadding = Vec4();
			d.comboBox.size[1] = 20;
			d.comboBox.listOffset = -1;
			d.comboBox.itemSpacing = -2;
			d.comboBox.itemsFormat.size = d.comboBox.selectedFormat.size = d.comboBox.placeholderFormat.size = 11;

			d.progressBar.textPadding = Vec4();
			d.progressBar.size[1] = 20;
			d.progressBar.textFormat.size = 11;

			d.sliderBar.horizontal.padding = Vec4(-1);
			d.sliderBar.vertical.padding = Vec4(-1);
			d.sliderBar.horizontal.margin = Vec4(1);
			d.sliderBar.vertical.margin = Vec4(1);
			d.sliderBar.horizontal.size[1] = 20;
			d.sliderBar.vertical.size[0] = 20;

			d.colorPicker.collapsedSize = Vec2(20);
			d.colorPicker.fullSize *= 20.0 / 28.0;

			d.panel.contentPadding = Vec4(1);
			d.panel.captionPadding = Vec4(1);
			d.panel.captionHeight = 22;
			d.panel.textFormat.size = 11;

			d.spoiler.contentPadding = Vec4(1);
			d.spoiler.captionPadding = Vec4(1);
			d.spoiler.captionHeight = 22;
			d.spoiler.textFormat.size = 11;
		}
	}

	namespace detail
	{
		GuiSkinConfig guiSkinGenerate(GuiSkinIndex style)
		{
			GuiSkinConfig skin;
			switch (style.index)
			{
				case GuiSkinLarge.index:
					generateLarge(skin);
					break;
				case GuiSkinCompact.index:
					generateCompact(skin);
					break;
				case GuiSkinTooltips.index:
					generateCompact(skin);
					skin.textureName = HashString("cage/textures/tooltips.png");
					break;
			}
			return skin;
		}
	}
}
