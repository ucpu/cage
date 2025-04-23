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
			text.size = 13;
			text.align = TextAlignEnum::Left;
			text.lineSpacing = 1;
			return text;
		}

		consteval GuiImageFormatComponent imageFormatComponentInit()
		{
			GuiImageFormatComponent Image;
			Image.animationOffset = 0;
			Image.animationSpeed = 1;
			Image.mode = ImageModeEnum::Fit;
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
					GuiElementTypeEnum::PanelBase,
					GuiElementTypeEnum::Frame,
					GuiElementTypeEnum::SpoilerBase,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::TextArea,
					GuiElementTypeEnum::InvalidElement,
				};

				std::vector<GuiElementTypeEnum> smallElements = {
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
					GuiElementTypeEnum::ComboBoxBase,
					GuiElementTypeEnum::ComboBoxIconCollapsed,
					GuiElementTypeEnum::ComboBoxIconShown,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::ComboBoxList,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::ComboBoxItemUnchecked,
					GuiElementTypeEnum::ComboBoxItemChecked,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::CheckBoxUnchecked,
					GuiElementTypeEnum::CheckBoxChecked,
					GuiElementTypeEnum::CheckBoxIndetermined,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::RadioBoxUnchecked,
					GuiElementTypeEnum::RadioBoxChecked,
					GuiElementTypeEnum::RadioBoxIndetermined,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::ScrollbarHorizontalPanel,
					GuiElementTypeEnum::ScrollbarVerticalPanel,
					GuiElementTypeEnum::ScrollbarHorizontalDot,
					GuiElementTypeEnum::ScrollbarVerticalDot,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::SliderHorizontalPanel,
					GuiElementTypeEnum::SliderVerticalPanel,
					GuiElementTypeEnum::SliderHorizontalDot,
					GuiElementTypeEnum::SliderVerticalDot,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::ProgressBar,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::ColorPickerCompact,
					GuiElementTypeEnum::ColorPickerFull,
					GuiElementTypeEnum::ColorPickerHuePanel,
					GuiElementTypeEnum::ColorPickerSatValPanel,
					GuiElementTypeEnum::ColorPickerPreviewPanel,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::Header,
					GuiElementTypeEnum::InvalidElement,
					GuiElementTypeEnum::SeparatorHorizontalLine /* overlaps with separator vertical line */,
					GuiElementTypeEnum::InvalidElement,
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
					GuiElementTypeEnum::Header,
					GuiElementTypeEnum::SeparatorHorizontalLine,
				};

				FlatSet<GuiElementTypeEnum> noBorder = {
					GuiElementTypeEnum::SpoilerIconCollapsed,
					GuiElementTypeEnum::SpoilerIconShown,
					GuiElementTypeEnum::ComboBoxIconCollapsed,
					GuiElementTypeEnum::ComboBoxIconShown,
					GuiElementTypeEnum::RadioBoxChecked,
					GuiElementTypeEnum::RadioBoxUnchecked,
					GuiElementTypeEnum::RadioBoxIndetermined,
					GuiElementTypeEnum::SliderHorizontalDot,
					GuiElementTypeEnum::SliderVerticalDot,
				};

				FlatSet<GuiElementTypeEnum> noFocus = {
					GuiElementTypeEnum::PanelBase,
					GuiElementTypeEnum::PanelCaption,
					GuiElementTypeEnum::Frame,
					GuiElementTypeEnum::SpoilerBase,
					GuiElementTypeEnum::SpoilerCaption,
					GuiElementTypeEnum::SpoilerIconCollapsed,
					GuiElementTypeEnum::SpoilerIconShown,
					GuiElementTypeEnum::InputButtonDecrement,
					GuiElementTypeEnum::InputButtonIncrement,
					GuiElementTypeEnum::ComboBoxList,
					GuiElementTypeEnum::ComboBoxItemUnchecked,
					GuiElementTypeEnum::ComboBoxItemChecked,
					GuiElementTypeEnum::ComboBoxIconCollapsed,
					GuiElementTypeEnum::ComboBoxIconShown,
					GuiElementTypeEnum::ProgressBar,
					GuiElementTypeEnum::ColorPickerHuePanel,
					GuiElementTypeEnum::ColorPickerPreviewPanel,
					GuiElementTypeEnum::ColorPickerSatValPanel,
					GuiElementTypeEnum::Header,
					GuiElementTypeEnum::SeparatorHorizontalLine,
					GuiElementTypeEnum::SeparatorVerticalLine,
				};

				FlatSet<GuiElementTypeEnum> noHover = {
					GuiElementTypeEnum::PanelBase,
					GuiElementTypeEnum::Frame,
					GuiElementTypeEnum::SpoilerBase,
					GuiElementTypeEnum::SpoilerIconCollapsed,
					GuiElementTypeEnum::SpoilerIconShown,
					GuiElementTypeEnum::ComboBoxList,
					GuiElementTypeEnum::ComboBoxIconCollapsed,
					GuiElementTypeEnum::ComboBoxIconShown,
					GuiElementTypeEnum::ColorPickerHuePanel,
					GuiElementTypeEnum::ColorPickerPreviewPanel,
					GuiElementTypeEnum::ColorPickerSatValPanel,
					GuiElementTypeEnum::Header,
					GuiElementTypeEnum::SeparatorHorizontalLine,
					GuiElementTypeEnum::SeparatorVerticalLine,
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
				}
			}
		};

		constexpr const LayoutsBase layoutsBase;
	}

	GuiSkinConfig::GuiSkinConfig() : textureId(HashString("cage/textures/gui.png")), hoverSound(HashString("cage/sounds/hover.wav")), openTooltipSound(HashString("cage/sounds/tooltipOpen.wav"))
	{
		std::copy(std::begin(layoutsBase.layouts), std::end(layoutsBase.layouts), std::begin(layouts));
	}

	GuiSkinWidgetDefaults::Label::Label() : textFormat(TextInit), imageFormat(ImageInit) {}

	GuiSkinWidgetDefaults::Header::Header() : textFormat(TextInit), imageFormat(ImageInit)
	{
		textFormat.size = 16;
		textFormat.align = TextAlignEnum::Center;
	}

	GuiSkinWidgetDefaults::Separator::Separator()
	{
		horizontal.size = Vec2(150, 20);
		vertical.size = Vec2(20, 150);
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

	GuiSkinWidgetDefaults::CheckBox::CheckBox() : textFormat(TextInit), clickSound(ClickSound) {}

	GuiSkinWidgetDefaults::RadioBox::RadioBox() : textFormat(TextInit), clickSound(ClickSound) {}

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
		horizontal.size = Vec2(150, 24);
		vertical.size = Vec2(24, 150);
	}

	GuiSkinWidgetDefaults::ColorPicker::ColorPicker() : slidingSound(SlidingSound) {}

	GuiSkinWidgetDefaults::SolidColor::SolidColor() {}

	GuiSkinWidgetDefaults::Frame::Frame() {}

	GuiSkinWidgetDefaults::Panel::Panel() : textFormat(TextInit), imageFormat(ImageInit)
	{
		textFormat.align = TextAlignEnum::Center;
	}

	GuiSkinWidgetDefaults::Spoiler::Spoiler() : textFormat(TextInit), imageFormat(ImageInit), clickSound(ClickSound)
	{
		textFormat.align = TextAlignEnum::Center;
	}

	GuiSkinWidgetDefaults::Scrollbars::Scrollbars() : slidingSound(SlidingSound) {}

	namespace detail
	{
		GuiSkinConfig guiSkinGenerate(GuiSkinIndex style)
		{
			GuiSkinConfig skin;
			if (style.index == GuiSkinTooltips.index)
				skin.textureId = HashString("cage/textures/tooltips.png");
			return skin;
		}
	}
}
