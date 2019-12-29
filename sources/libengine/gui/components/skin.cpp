#include <set>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/hashString.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/gui.h>
#include <cage-engine/graphics.h>
#include <cage-engine/window.h>
#include "../private.h"

namespace cage
{
	namespace
	{
		TextFormatComponent textFormatComponentInit()
		{
			TextFormatComponent text;
			text.color = vec3(1, 1, 1);
			text.font = HashString("cage/font/ubuntu/Ubuntu-R.ttf");
			text.align = TextAlignEnum::Left;
			text.lineSpacing = 0;
			return text;
		}

		ImageFormatComponent imageFormatComponentInit()
		{
			ImageFormatComponent Image;
			Image.animationOffset = 0;
			Image.animationSpeed = 1;
			Image.mode = ImageModeEnum::Stretch;
			return Image;
		}

		const TextFormatComponent textInit = textFormatComponentInit();
		const ImageFormatComponent imageInit = imageFormatComponentInit();
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
		std::vector<ElementTypeEnum> largeElements = {
			ElementTypeEnum::PanelBase, // overlaps SpoilerBase
			ElementTypeEnum::TextArea,
		};

		std::vector<ElementTypeEnum> smallElements = {
			ElementTypeEnum::ScrollbarHorizontalPanel,
			ElementTypeEnum::ScrollbarVerticalPanel,
			ElementTypeEnum::ScrollbarHorizontalDot,
			ElementTypeEnum::ScrollbarVerticalDot,
			ElementTypeEnum::InvalidElement,
			ElementTypeEnum::ToolTip,
			ElementTypeEnum::InvalidElement,
			ElementTypeEnum::PanelCaption,
			ElementTypeEnum::InvalidElement,
			ElementTypeEnum::SpoilerCaption,
			ElementTypeEnum::SpoilerIconCollapsed,
			ElementTypeEnum::SpoilerIconShown,
			ElementTypeEnum::InvalidElement,
			ElementTypeEnum::Button,
			ElementTypeEnum::InvalidElement,
			ElementTypeEnum::Input,
			ElementTypeEnum::InputButtonDecrement,
			ElementTypeEnum::InputButtonIncrement,
			ElementTypeEnum::InvalidElement,
			ElementTypeEnum::CheckBoxUnchecked,
			ElementTypeEnum::CheckBoxChecked,
			ElementTypeEnum::CheckBoxIndetermined,
			ElementTypeEnum::InvalidElement,
			ElementTypeEnum::RadioBoxUnchecked,
			ElementTypeEnum::RadioBoxChecked,
			ElementTypeEnum::RadioBoxIndetermined,
			ElementTypeEnum::InvalidElement,
			ElementTypeEnum::ComboBoxBase,
			ElementTypeEnum::ComboBoxList,
			ElementTypeEnum::InvalidElement,
			ElementTypeEnum::ListBoxBase,
			ElementTypeEnum::InvalidElement,
			ElementTypeEnum::ListBoxItemUnchecked,
			ElementTypeEnum::ComboBoxItemUnchecked,
			ElementTypeEnum::InvalidElement,
			ElementTypeEnum::ListBoxItemChecked,
			ElementTypeEnum::ComboBoxItemChecked,
			ElementTypeEnum::InvalidElement,
			ElementTypeEnum::SliderHorizontalPanel,
			ElementTypeEnum::SliderVerticalPanel,
			ElementTypeEnum::SliderHorizontalDot,
			ElementTypeEnum::SliderVerticalDot,
			ElementTypeEnum::InvalidElement,
			ElementTypeEnum::ProgressBar,
			ElementTypeEnum::InvalidElement,
			ElementTypeEnum::ColorPickerCompact, // overlaps: ColorPickerFull
			ElementTypeEnum::ColorPickerPreviewPanel, // overlaps: ColorPickerHuePanel, ColorPickerSatValPanel
		};

		std::set<ElementTypeEnum> wideElements = {
			ElementTypeEnum::PanelCaption,
			ElementTypeEnum::SpoilerCaption,
			ElementTypeEnum::Button,
			ElementTypeEnum::Input,
			ElementTypeEnum::ComboBoxBase,
			ElementTypeEnum::ComboBoxList,
			ElementTypeEnum::ListBoxBase,
			ElementTypeEnum::ListBoxItemUnchecked,
			ElementTypeEnum::ComboBoxItemUnchecked,
			ElementTypeEnum::ListBoxItemChecked,
			ElementTypeEnum::ComboBoxItemChecked,
			ElementTypeEnum::ToolTip,
			ElementTypeEnum::ProgressBar,
		};

		std::set<ElementTypeEnum> noBorder = {
			//ElementTypeEnum::ScrollbarHorizontalDot, ElementTypeEnum::ScrollbarVerticalDot,
			ElementTypeEnum::SpoilerIconCollapsed, ElementTypeEnum::SpoilerIconShown,
			ElementTypeEnum::RadioBoxChecked, ElementTypeEnum::RadioBoxUnchecked, ElementTypeEnum::RadioBoxIndetermined,
			ElementTypeEnum::SliderHorizontalDot, ElementTypeEnum::SliderVerticalDot,
		};

		std::set<ElementTypeEnum> noFocus = {
			ElementTypeEnum::PanelBase, ElementTypeEnum::PanelCaption,
			ElementTypeEnum::SpoilerBase, ElementTypeEnum::SpoilerCaption, ElementTypeEnum::SpoilerIconCollapsed, ElementTypeEnum::SpoilerIconShown,
			ElementTypeEnum::InputButtonDecrement, ElementTypeEnum::InputButtonIncrement,
			ElementTypeEnum::ComboBoxList, ElementTypeEnum::ComboBoxItemUnchecked, ElementTypeEnum::ComboBoxItemChecked,
			ElementTypeEnum::ListBoxItemUnchecked, ElementTypeEnum::ListBoxItemChecked,
			ElementTypeEnum::ProgressBar,
			ElementTypeEnum::ColorPickerHuePanel, ElementTypeEnum::ColorPickerPreviewPanel, ElementTypeEnum::ColorPickerSatValPanel,
			ElementTypeEnum::ToolTip,
		};

		std::set<ElementTypeEnum> noHover = {
			ElementTypeEnum::PanelBase,
			ElementTypeEnum::SpoilerBase, ElementTypeEnum::SpoilerIconCollapsed, ElementTypeEnum::SpoilerIconShown,
			ElementTypeEnum::ComboBoxList,
			ElementTypeEnum::ColorPickerHuePanel, ElementTypeEnum::ColorPickerPreviewPanel, ElementTypeEnum::ColorPickerSatValPanel,
			ElementTypeEnum::ToolTip,
		};

		{ // automatic uv construction
			packerStruct packer;
			packer.frame = 4.f / 1024;
			packer.border = 4.f / 1024;

			// large
			packer.size = vec2(5, 5) * 24.f / 1024;
			for (auto it : largeElements)
			{
				if (it == ElementTypeEnum::InvalidElement)
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
				if (it == ElementTypeEnum::InvalidElement)
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
			layouts[(uint32)ElementTypeEnum::ColorPickerFull].textureUv = layouts[(uint32)ElementTypeEnum::ColorPickerCompact].textureUv;
			layouts[(uint32)ElementTypeEnum::ColorPickerHuePanel].textureUv = layouts[(uint32)ElementTypeEnum::ColorPickerPreviewPanel].textureUv;
			layouts[(uint32)ElementTypeEnum::ColorPickerSatValPanel].textureUv = layouts[(uint32)ElementTypeEnum::ColorPickerPreviewPanel].textureUv;
			layouts[(uint32)ElementTypeEnum::SpoilerBase].textureUv = layouts[(uint32)ElementTypeEnum::PanelBase].textureUv;
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
