#include <cage-core/hashString.h>
#include <cage-core/flatSet.h>

#include "private.h"

namespace cage
{
	namespace
	{
		GuiTextFormatComponent textFormatComponentInit()
		{
			GuiTextFormatComponent text;
			text.color = Vec3(1);
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

	namespace
	{
		struct Packer
		{
			Vec2 size; // including border
			Real frame; // spacing around element
			Real border; // border inside element

			GuiSkinElementLayout::TextureUvOi next(bool allowBorder)
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

			void newLine()
			{
				p[0] = 0;
				p[1] = my;
			}

		private:
			Vec2 p;
			Real my;
		};
	}

	GuiSkinConfig::GuiSkinConfig() : textureName(HashString("cage/texture/gui.png"))
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

		FlatSet<GuiElementTypeEnum> wideElements = {
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

		FlatSet<GuiElementTypeEnum> noBorder = {
			//GuiElementTypeEnum::ScrollbarHorizontalDot, GuiElementTypeEnum::ScrollbarVerticalDot,
			GuiElementTypeEnum::SpoilerIconCollapsed, GuiElementTypeEnum::SpoilerIconShown,
			GuiElementTypeEnum::RadioBoxChecked, GuiElementTypeEnum::RadioBoxUnchecked, GuiElementTypeEnum::RadioBoxIndetermined,
			GuiElementTypeEnum::SliderHorizontalDot, GuiElementTypeEnum::SliderVerticalDot,
		};

		FlatSet<GuiElementTypeEnum> noFocus = {
			GuiElementTypeEnum::PanelBase, GuiElementTypeEnum::PanelCaption,
			GuiElementTypeEnum::SpoilerBase, GuiElementTypeEnum::SpoilerCaption, GuiElementTypeEnum::SpoilerIconCollapsed, GuiElementTypeEnum::SpoilerIconShown,
			GuiElementTypeEnum::InputButtonDecrement, GuiElementTypeEnum::InputButtonIncrement,
			GuiElementTypeEnum::ComboBoxList, GuiElementTypeEnum::ComboBoxItemUnchecked, GuiElementTypeEnum::ComboBoxItemChecked,
			GuiElementTypeEnum::ListBoxItemUnchecked, GuiElementTypeEnum::ListBoxItemChecked,
			GuiElementTypeEnum::ProgressBar,
			GuiElementTypeEnum::ColorPickerHuePanel, GuiElementTypeEnum::ColorPickerPreviewPanel, GuiElementTypeEnum::ColorPickerSatValPanel,
			GuiElementTypeEnum::ToolTip,
		};

		FlatSet<GuiElementTypeEnum> noHover = {
			GuiElementTypeEnum::PanelBase,
			GuiElementTypeEnum::SpoilerBase, GuiElementTypeEnum::SpoilerIconCollapsed, GuiElementTypeEnum::SpoilerIconShown,
			GuiElementTypeEnum::ComboBoxList,
			GuiElementTypeEnum::ColorPickerHuePanel, GuiElementTypeEnum::ColorPickerPreviewPanel, GuiElementTypeEnum::ColorPickerSatValPanel,
			GuiElementTypeEnum::ToolTip,
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
			layouts[(uint32)GuiElementTypeEnum::ColorPickerFull].textureUv = layouts[(uint32)GuiElementTypeEnum::ColorPickerCompact].textureUv;
			layouts[(uint32)GuiElementTypeEnum::ColorPickerHuePanel].textureUv = layouts[(uint32)GuiElementTypeEnum::ColorPickerPreviewPanel].textureUv;
			layouts[(uint32)GuiElementTypeEnum::ColorPickerSatValPanel].textureUv = layouts[(uint32)GuiElementTypeEnum::ColorPickerPreviewPanel].textureUv;
			layouts[(uint32)GuiElementTypeEnum::SpoilerBase].textureUv = layouts[(uint32)GuiElementTypeEnum::PanelBase].textureUv;
		}
	}

	GuiSkinWidgetDefaults::Label::Label() : textFormat(textInit), imageFormat(imageInit)
	{}

	GuiSkinWidgetDefaults::Button::Button() : textFormat(textInit), imageFormat(imageInit)
	{
		textFormat.align = TextAlignEnum::Center;
	}

	GuiSkinWidgetDefaults::Input::Input() : textValidFormat(textInit), textInvalidFormat(textInit), placeholderFormat(textInit)
	{
		textInvalidFormat.color = Vec3(1,0,0);
		placeholderFormat.color = Vec3(0.5,0.5,0.5);
	}

	GuiSkinWidgetDefaults::TextArea::TextArea() : textFormat(textInit)
	{}

	GuiSkinWidgetDefaults::CheckBox::CheckBox() : textFormat(textInit)
	{}

	GuiSkinWidgetDefaults::RadioBox::RadioBox() : textFormat(textInit)
	{}

	GuiSkinWidgetDefaults::ComboBox::ComboBox() : placeholderFormat(textInit), itemsFormat(textInit), selectedFormat(textInit)
	{
		placeholderFormat.color = Vec3(0.5, 0.5, 0.5);
		placeholderFormat.align = TextAlignEnum::Center;
		itemsFormat.align = TextAlignEnum::Center;
		selectedFormat.align = TextAlignEnum::Center;
	}

	GuiSkinWidgetDefaults::ListBox::ListBox() : textFormat(textInit)
	{}

	GuiSkinWidgetDefaults::ProgressBar::ProgressBar() : textFormat(textInit), backgroundImageFormat(imageInit), fillingImageFormat(imageInit)
	{
		fillingImage.animationStart = 0;
		fillingImage.textureName = HashString("todo"); // todo
		fillingImage.textureUvOffset = Vec2();
		fillingImage.textureUvSize = Vec2(1);
	}

	GuiSkinWidgetDefaults::SliderBar::SliderBar()
	{
		horizontal.size = Vec2(150, 28);
		vertical.size = Vec2(28, 150);
	}

	GuiSkinWidgetDefaults::Panel::Panel() : textFormat(textInit), imageFormat(imageInit)
	{
		textFormat.align = TextAlignEnum::Center;
	}

	GuiSkinWidgetDefaults::Spoiler::Spoiler() : textFormat(textInit), imageFormat(imageInit)
	{
		textFormat.align = TextAlignEnum::Center;
	}

	GuiSkinWidgetDefaults::Tooltip::Tooltip() : textFormat(textInit)
	{}
}
