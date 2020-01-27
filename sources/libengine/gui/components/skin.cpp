#include <cage-core/hashString.h>

#include "../private.h"

#include <set>

namespace cage
{
	namespace
	{
		GuiTextFormatComponent textFormatComponentInit()
		{
			GuiTextFormatComponent text;
			text.color = vec3(1, 1, 1);
			text.font = HashString("cage/font/ubuntu/Ubuntu-R.ttf");
			text.align = TextAlignEnum::Left;
			text.lineSpacing = 0;
			return text;
		}

		GuiImageFormatComponent imageFormatComponentInit()
		{
			GuiImageFormatComponent Image;
			Image.animationOffset = 0;
			Image.animationSpeed = 1;
			Image.mode = ImageModeEnum::Stretch;
			return Image;
		}

		const GuiTextFormatComponent textInit = textFormatComponentInit();
		const GuiImageFormatComponent imageInit = imageFormatComponentInit();
	}

	GuiSkinElementLayout::TextureUv::TextureUv()
	{
		detail::memset(this, 0, sizeof(*this));
	}

	GuiSkinElementLayout::GuiSkinElementLayout() : border(5, 5, 5, 5)
	{}

	GuiSkinWidgetDefaults::GuiSkinWidgetDefaults()
	{}

	namespace
	{
		struct packerStruct
		{
			vec2 size; // including border
			real frame; // spacing around element
			real border; // border inside element

			GuiSkinElementLayout::TextureUvOi next(bool allowBorder)
			{
				vec2 s = size + frame * 2;
				CAGE_ASSERT(s[0] < 1, s, frame, border, size);
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
					CAGE_ASSERT(size[0] >= 2 * border && size[1] >= 2 * border, frame, border, size);
					r.inner[0] += border;
					r.inner[1] += border;
					r.inner[2] -= border;
					r.inner[3] -= border;
				}
				my = max(my, p[1] + s[1]);
				p[0] += s[0];
				return r;
			}

			void newLine()
			{
				p[0] = 0;
				p[1] = my;
			}

		private:
			vec2 p;
			real my;
		};
	}

	GuiSkinConfig::GuiSkinConfig() : textureName(HashString("cage/texture/gui.psd"))
	{
		std::vector<GuiElementTypeEnum> largeElements = {
			GuiElementTypeEnum::PanelBase, // overlaps SpoilerBase
			GuiElementTypeEnum::TextArea,
		};

		std::vector<GuiElementTypeEnum> smallElements = {
			GuiElementTypeEnum::ScrollbarHorizontalPanel,
			GuiElementTypeEnum::ScrollbarVerticalPanel,
			GuiElementTypeEnum::ScrollbarHorizontalDot,
			GuiElementTypeEnum::ScrollbarVerticalDot,
			GuiElementTypeEnum::InvalidElement,
			GuiElementTypeEnum::ToolTip,
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
			GuiElementTypeEnum::ComboBoxBase,
			GuiElementTypeEnum::ComboBoxList,
			GuiElementTypeEnum::InvalidElement,
			GuiElementTypeEnum::ListBoxBase,
			GuiElementTypeEnum::InvalidElement,
			GuiElementTypeEnum::ListBoxItemUnchecked,
			GuiElementTypeEnum::ComboBoxItemUnchecked,
			GuiElementTypeEnum::InvalidElement,
			GuiElementTypeEnum::ListBoxItemChecked,
			GuiElementTypeEnum::ComboBoxItemChecked,
			GuiElementTypeEnum::InvalidElement,
			GuiElementTypeEnum::SliderHorizontalPanel,
			GuiElementTypeEnum::SliderVerticalPanel,
			GuiElementTypeEnum::SliderHorizontalDot,
			GuiElementTypeEnum::SliderVerticalDot,
			GuiElementTypeEnum::InvalidElement,
			GuiElementTypeEnum::ProgressBar,
			GuiElementTypeEnum::InvalidElement,
			GuiElementTypeEnum::ColorPickerCompact, // overlaps: ColorPickerFull
			GuiElementTypeEnum::ColorPickerPreviewPanel, // overlaps: ColorPickerHuePanel, ColorPickerSatValPanel
		};

		std::set<GuiElementTypeEnum> wideElements = {
			GuiElementTypeEnum::PanelCaption,
			GuiElementTypeEnum::SpoilerCaption,
			GuiElementTypeEnum::Button,
			GuiElementTypeEnum::Input,
			GuiElementTypeEnum::ComboBoxBase,
			GuiElementTypeEnum::ComboBoxList,
			GuiElementTypeEnum::ListBoxBase,
			GuiElementTypeEnum::ListBoxItemUnchecked,
			GuiElementTypeEnum::ComboBoxItemUnchecked,
			GuiElementTypeEnum::ListBoxItemChecked,
			GuiElementTypeEnum::ComboBoxItemChecked,
			GuiElementTypeEnum::ToolTip,
			GuiElementTypeEnum::ProgressBar,
		};

		std::set<GuiElementTypeEnum> noBorder = {
			//GuiElementTypeEnum::ScrollbarHorizontalDot, GuiElementTypeEnum::ScrollbarVerticalDot,
			GuiElementTypeEnum::SpoilerIconCollapsed, GuiElementTypeEnum::SpoilerIconShown,
			GuiElementTypeEnum::RadioBoxChecked, GuiElementTypeEnum::RadioBoxUnchecked, GuiElementTypeEnum::RadioBoxIndetermined,
			GuiElementTypeEnum::SliderHorizontalDot, GuiElementTypeEnum::SliderVerticalDot,
		};

		std::set<GuiElementTypeEnum> noFocus = {
			GuiElementTypeEnum::PanelBase, GuiElementTypeEnum::PanelCaption,
			GuiElementTypeEnum::SpoilerBase, GuiElementTypeEnum::SpoilerCaption, GuiElementTypeEnum::SpoilerIconCollapsed, GuiElementTypeEnum::SpoilerIconShown,
			GuiElementTypeEnum::InputButtonDecrement, GuiElementTypeEnum::InputButtonIncrement,
			GuiElementTypeEnum::ComboBoxList, GuiElementTypeEnum::ComboBoxItemUnchecked, GuiElementTypeEnum::ComboBoxItemChecked,
			GuiElementTypeEnum::ListBoxItemUnchecked, GuiElementTypeEnum::ListBoxItemChecked,
			GuiElementTypeEnum::ProgressBar,
			GuiElementTypeEnum::ColorPickerHuePanel, GuiElementTypeEnum::ColorPickerPreviewPanel, GuiElementTypeEnum::ColorPickerSatValPanel,
			GuiElementTypeEnum::ToolTip,
		};

		std::set<GuiElementTypeEnum> noHover = {
			GuiElementTypeEnum::PanelBase,
			GuiElementTypeEnum::SpoilerBase, GuiElementTypeEnum::SpoilerIconCollapsed, GuiElementTypeEnum::SpoilerIconShown,
			GuiElementTypeEnum::ComboBoxList,
			GuiElementTypeEnum::ColorPickerHuePanel, GuiElementTypeEnum::ColorPickerPreviewPanel, GuiElementTypeEnum::ColorPickerSatValPanel,
			GuiElementTypeEnum::ToolTip,
		};

		{ // automatic uv construction
			packerStruct packer;
			packer.frame = 4.f / 1024;
			packer.border = 4.f / 1024;

			// large
			packer.size = vec2(5, 5) * 24.f / 1024;
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
					packer.size = vec2(5, 1) * 24.f / 1024;
				else
					packer.size = vec2(1, 1) * 24.f / 1024;
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
				layouts[(uint32)it].border = vec4();
		}

		{ // overlaps
			layouts[(uint32)GuiElementTypeEnum::ColorPickerFull].textureUv = layouts[(uint32)GuiElementTypeEnum::ColorPickerCompact].textureUv;
			layouts[(uint32)GuiElementTypeEnum::ColorPickerHuePanel].textureUv = layouts[(uint32)GuiElementTypeEnum::ColorPickerPreviewPanel].textureUv;
			layouts[(uint32)GuiElementTypeEnum::ColorPickerSatValPanel].textureUv = layouts[(uint32)GuiElementTypeEnum::ColorPickerPreviewPanel].textureUv;
			layouts[(uint32)GuiElementTypeEnum::SpoilerBase].textureUv = layouts[(uint32)GuiElementTypeEnum::PanelBase].textureUv;
		}
	}

	GuiSkinWidgetDefaults::Label::Label() : textFormat(textInit), imageFormat(imageInit), margin(1, 2, 1, 2)
	{}

	GuiSkinWidgetDefaults::Button::Button() : textFormat(textInit), imageFormat(imageInit), padding(1, 1, 1, 1), margin(1, 1, 1, 1), size(150, 32)
	{
		textFormat.align = TextAlignEnum::Center;
	}

	GuiSkinWidgetDefaults::Input::Input() : textValidFormat(textInit), textInvalidFormat(textInit), placeholderFormat(textInit), basePadding(2, 2, 2, 2), margin(1, 1, 1, 1), size(300, 32), buttonsSize(32), buttonsOffset(2), buttonsMode(InputButtonsPlacementModeEnum::Sides)
	{
		textInvalidFormat.color = vec3(1,0,0);
		placeholderFormat.color = vec3(0.5,0.5,0.5);
	}

	GuiSkinWidgetDefaults::TextArea::TextArea() : textFormat(textInit), padding(3, 3, 3, 3), margin(1, 1, 1, 1), size(450, 200)
	{}

	GuiSkinWidgetDefaults::CheckBox::CheckBox() : textFormat(textInit), margin(1, 1, 1, 1), size(28, 28), labelOffset(3, 5)
	{}

	GuiSkinWidgetDefaults::RadioBox::RadioBox() : textFormat(textInit), margin(1, 1, 1, 1), size(28, 28), labelOffset(3, 5)
	{}

	GuiSkinWidgetDefaults::ComboBox::ComboBox() : placeholderFormat(textInit), itemsFormat(textInit), selectedFormat(textInit), basePadding(1, 1, 1, 1), baseMargin(1, 1, 1, 1), listPadding(0, 0, 0, 0), itemPadding(1, 2, 1, 2), size(250, 32), listOffset(-6), itemSpacing(-2)
	{
		placeholderFormat.color = vec3(0.5, 0.5, 0.5);
		placeholderFormat.align = TextAlignEnum::Center;
		itemsFormat.align = TextAlignEnum::Center;
		selectedFormat.align = TextAlignEnum::Center;
	}

	GuiSkinWidgetDefaults::ListBox::ListBox() : textFormat(textInit), basePadding(0, 0, 0, 0), baseMargin(1, 1, 1, 1), itemPadding(1, 1, 1, 1), size(250, 32), itemSpacing(0)
	{}

	GuiSkinWidgetDefaults::ProgressBar::ProgressBar() : textFormat(textInit), backgroundImageFormat(imageInit), fillingImageFormat(imageInit), baseMargin(1, 1, 1, 1), textPadding(1, 1, 1, 1), fillingPadding(1, 1, 1, 1), size(200, 28)
	{
		fillingImage.animationStart = 0;
		fillingImage.textureName = HashString("todo");
		fillingImage.textureUvOffset = vec2();
		fillingImage.textureUvSize = vec2(1, 1);
	}

	GuiSkinWidgetDefaults::SliderBar::SliderBar()
	{
		horizontal.padding = vec4(1, 1, 1, 1);
		horizontal.margin = vec4(1, 1, 1, 1);
		horizontal.size = vec2(150, 28);
		horizontal.collapsedBar = true;
		vertical.padding = vec4(1, 1, 1, 1);
		vertical.margin = vec4(1, 1, 1, 1);
		vertical.size = vec2(28, 150);
		vertical.collapsedBar = true;
	}

	GuiSkinWidgetDefaults::ColorPicker::ColorPicker() : margin(1, 1, 1, 1), collapsedSize(40, 32), fullSize(250, 180), hueBarPortion(0.18), resultBarPortion(0.35)
	{}

	GuiSkinWidgetDefaults::Panel::Panel() : textFormat(textInit), imageFormat(imageInit), contentPadding(2, 2, 2, 2), baseMargin(1, 1, 1, 1), captionPadding(3, 1, 3, 1), captionHeight(28)
	{
		textFormat.align = TextAlignEnum::Center;
	}

	GuiSkinWidgetDefaults::Spoiler::Spoiler() : textFormat(textInit), imageFormat(imageInit), contentPadding(2, 2, 2, 2), baseMargin(1, 1, 1, 1), captionPadding(3, 1, 3, 1), captionHeight(28)
	{
		textFormat.align = TextAlignEnum::Center;
	}

	GuiSkinWidgetDefaults::Scrollbars::Scrollbars() : scrollbarSize(15), contentPadding(4)
	{}

	GuiSkinWidgetDefaults::Tooltip::Tooltip() : textFormat(textInit)
	{}
}
