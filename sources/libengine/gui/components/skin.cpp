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
		textFormatComponent textFormatComponentInit()
		{
			textFormatComponent text;
			text.color = vec3(1, 1, 1);
			text.font = hashString("cage/font/ubuntu/Ubuntu-R.ttf");
			text.align = textAlignEnum::Left;
			text.lineSpacing = 0;
			return text;
		}

		imageFormatComponent imageFormatComponentInit()
		{
			imageFormatComponent image;
			image.animationOffset = 0;
			image.animationSpeed = 1;
			image.mode = imageModeEnum::Stretch;
			return image;
		}

		const textFormatComponent textInit = textFormatComponentInit();
		const imageFormatComponent imageInit = imageFormatComponentInit();
	}

	guiSkinElementLayout::textureUvStruct::textureUvStruct()
	{
		detail::memset(this, 0, sizeof(*this));
	}

	guiSkinElementLayout::guiSkinElementLayout() : border(5, 5, 5, 5)
	{}

	guiSkinWidgetDefaults::guiSkinWidgetDefaults()
	{}

	namespace
	{
		struct packerStruct
		{
			vec2 size; // including border
			real frame; // spacing around element
			real border; // border inside element

			guiSkinElementLayout::textureUvOiStruct next(bool allowBorder)
			{
				vec2 s = size + frame * 2;
				CAGE_ASSERT(s[0] < 1, s, frame, border, size);
				if (p[0] + s[0] > 1)
					newLine();
				guiSkinElementLayout::textureUvOiStruct r;
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

	guiSkinConfig::guiSkinConfig() : textureName(hashString("cage/texture/gui.psd"))
	{
		std::vector<elementTypeEnum> largeElements = {
			elementTypeEnum::PanelBase, // overlaps SpoilerBase
			elementTypeEnum::TextArea,
		};

		std::vector<elementTypeEnum> smallElements = {
			elementTypeEnum::ScrollbarHorizontalPanel,
			elementTypeEnum::ScrollbarVerticalPanel,
			elementTypeEnum::ScrollbarHorizontalDot,
			elementTypeEnum::ScrollbarVerticalDot,
			elementTypeEnum::InvalidElement,
			elementTypeEnum::ToolTip,
			elementTypeEnum::InvalidElement,
			elementTypeEnum::PanelCaption,
			elementTypeEnum::InvalidElement,
			elementTypeEnum::SpoilerCaption,
			elementTypeEnum::SpoilerIconCollapsed,
			elementTypeEnum::SpoilerIconShown,
			elementTypeEnum::InvalidElement,
			elementTypeEnum::Button,
			elementTypeEnum::InvalidElement,
			elementTypeEnum::Input,
			elementTypeEnum::InputButtonDecrement,
			elementTypeEnum::InputButtonIncrement,
			elementTypeEnum::InvalidElement,
			elementTypeEnum::CheckBoxUnchecked,
			elementTypeEnum::CheckBoxChecked,
			elementTypeEnum::CheckBoxIndetermined,
			elementTypeEnum::InvalidElement,
			elementTypeEnum::RadioBoxUnchecked,
			elementTypeEnum::RadioBoxChecked,
			elementTypeEnum::RadioBoxIndetermined,
			elementTypeEnum::InvalidElement,
			elementTypeEnum::ComboBoxBase,
			elementTypeEnum::ComboBoxList,
			elementTypeEnum::InvalidElement,
			elementTypeEnum::ListBoxBase,
			elementTypeEnum::InvalidElement,
			elementTypeEnum::ListBoxItemUnchecked,
			elementTypeEnum::ComboBoxItemUnchecked,
			elementTypeEnum::InvalidElement,
			elementTypeEnum::ListBoxItemChecked,
			elementTypeEnum::ComboBoxItemChecked,
			elementTypeEnum::InvalidElement,
			elementTypeEnum::SliderHorizontalPanel,
			elementTypeEnum::SliderVerticalPanel,
			elementTypeEnum::SliderHorizontalDot,
			elementTypeEnum::SliderVerticalDot,
			elementTypeEnum::InvalidElement,
			elementTypeEnum::ProgressBar,
			elementTypeEnum::InvalidElement,
			elementTypeEnum::ColorPickerCompact, // overlaps: ColorPickerFull
			elementTypeEnum::ColorPickerPreviewPanel, // overlaps: ColorPickerHuePanel, ColorPickerSatValPanel
		};

		std::set<elementTypeEnum> wideElements = {
			elementTypeEnum::PanelCaption,
			elementTypeEnum::SpoilerCaption,
			elementTypeEnum::Button,
			elementTypeEnum::Input,
			elementTypeEnum::ComboBoxBase,
			elementTypeEnum::ComboBoxList,
			elementTypeEnum::ListBoxBase,
			elementTypeEnum::ListBoxItemUnchecked,
			elementTypeEnum::ComboBoxItemUnchecked,
			elementTypeEnum::ListBoxItemChecked,
			elementTypeEnum::ComboBoxItemChecked,
			elementTypeEnum::ToolTip,
			elementTypeEnum::ProgressBar,
		};

		std::set<elementTypeEnum> noBorder = {
			//elementTypeEnum::ScrollbarHorizontalDot, elementTypeEnum::ScrollbarVerticalDot,
			elementTypeEnum::SpoilerIconCollapsed, elementTypeEnum::SpoilerIconShown,
			elementTypeEnum::RadioBoxChecked, elementTypeEnum::RadioBoxUnchecked, elementTypeEnum::RadioBoxIndetermined,
			elementTypeEnum::SliderHorizontalDot, elementTypeEnum::SliderVerticalDot,
		};

		std::set<elementTypeEnum> noFocus = {
			elementTypeEnum::PanelBase, elementTypeEnum::PanelCaption,
			elementTypeEnum::SpoilerBase, elementTypeEnum::SpoilerCaption, elementTypeEnum::SpoilerIconCollapsed, elementTypeEnum::SpoilerIconShown,
			elementTypeEnum::InputButtonDecrement, elementTypeEnum::InputButtonIncrement,
			elementTypeEnum::ComboBoxList, elementTypeEnum::ComboBoxItemUnchecked, elementTypeEnum::ComboBoxItemChecked,
			elementTypeEnum::ListBoxItemUnchecked, elementTypeEnum::ListBoxItemChecked,
			elementTypeEnum::ProgressBar,
			elementTypeEnum::ColorPickerHuePanel, elementTypeEnum::ColorPickerPreviewPanel, elementTypeEnum::ColorPickerSatValPanel,
			elementTypeEnum::ToolTip,
		};

		std::set<elementTypeEnum> noHover = {
			elementTypeEnum::PanelBase,
			elementTypeEnum::SpoilerBase, elementTypeEnum::SpoilerIconCollapsed, elementTypeEnum::SpoilerIconShown,
			elementTypeEnum::ComboBoxList,
			elementTypeEnum::ColorPickerHuePanel, elementTypeEnum::ColorPickerPreviewPanel, elementTypeEnum::ColorPickerSatValPanel,
			elementTypeEnum::ToolTip,
		};

		{ // automatic uv construction
			packerStruct packer;
			packer.frame = 4.f / 1024;
			packer.border = 4.f / 1024;

			// large
			packer.size = vec2(5, 5) * 24.f / 1024;
			for (auto it : largeElements)
			{
				if (it == elementTypeEnum::InvalidElement)
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
				if (it == elementTypeEnum::InvalidElement)
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
			layouts[(uint32)elementTypeEnum::ColorPickerFull].textureUv = layouts[(uint32)elementTypeEnum::ColorPickerCompact].textureUv;
			layouts[(uint32)elementTypeEnum::ColorPickerHuePanel].textureUv = layouts[(uint32)elementTypeEnum::ColorPickerPreviewPanel].textureUv;
			layouts[(uint32)elementTypeEnum::ColorPickerSatValPanel].textureUv = layouts[(uint32)elementTypeEnum::ColorPickerPreviewPanel].textureUv;
			layouts[(uint32)elementTypeEnum::SpoilerBase].textureUv = layouts[(uint32)elementTypeEnum::PanelBase].textureUv;
		}
	}

	guiSkinWidgetDefaults::labelStruct::labelStruct() : textFormat(textInit), imageFormat(imageInit), margin(1, 2, 1, 2)
	{}

	guiSkinWidgetDefaults::buttonStruct::buttonStruct() : textFormat(textInit), imageFormat(imageInit), padding(1, 1, 1, 1), margin(1, 1, 1, 1), size(150, 32)
	{
		textFormat.align = textAlignEnum::Center;
	}

	guiSkinWidgetDefaults::inputStruct::inputStruct() : textValidFormat(textInit), textInvalidFormat(textInit), placeholderFormat(textInit), basePadding(2, 2, 2, 2), margin(1, 1, 1, 1), size(300, 32), buttonsSize(32), buttonsOffset(2), buttonsMode(inputButtonsPlacementModeEnum::Sides)
	{
		textInvalidFormat.color = vec3(1,0,0);
		placeholderFormat.color = vec3(0.5,0.5,0.5);
	}

	guiSkinWidgetDefaults::textAreaStruct::textAreaStruct() : textFormat(textInit), padding(3, 3, 3, 3), margin(1, 1, 1, 1), size(450, 200)
	{}

	guiSkinWidgetDefaults::checkBoxStruct::checkBoxStruct() : textFormat(textInit), margin(1, 1, 1, 1), size(28, 28), labelOffset(3, 5)
	{}

	guiSkinWidgetDefaults::radioBoxStruct::radioBoxStruct() : textFormat(textInit), margin(1, 1, 1, 1), size(28, 28), labelOffset(3, 5)
	{}

	guiSkinWidgetDefaults::comboBoxStruct::comboBoxStruct() : placeholderFormat(textInit), itemsFormat(textInit), selectedFormat(textInit), basePadding(1, 1, 1, 1), baseMargin(1, 1, 1, 1), listPadding(0, 0, 0, 0), itemPadding(1, 2, 1, 2), size(250, 32), listOffset(-6), itemSpacing(-2)
	{
		placeholderFormat.color = vec3(0.5, 0.5, 0.5);
		placeholderFormat.align = textAlignEnum::Center;
		itemsFormat.align = textAlignEnum::Center;
		selectedFormat.align = textAlignEnum::Center;
	}

	guiSkinWidgetDefaults::listBoxStruct::listBoxStruct() : textFormat(textInit), basePadding(0, 0, 0, 0), baseMargin(1, 1, 1, 1), itemPadding(1, 1, 1, 1), size(250, 32), itemSpacing(0)
	{}

	guiSkinWidgetDefaults::progressBarStruct::progressBarStruct() : textFormat(textInit), backgroundImageFormat(imageInit), fillingImageFormat(imageInit), baseMargin(1, 1, 1, 1), textPadding(1, 1, 1, 1), fillingPadding(1, 1, 1, 1), size(200, 28)
	{
		fillingImage.animationStart = 0;
		fillingImage.textureName = hashString("todo");
		fillingImage.textureUvOffset = vec2();
		fillingImage.textureUvSize = vec2(1, 1);
	}

	guiSkinWidgetDefaults::sliderBarStruct::sliderBarStruct()
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

	guiSkinWidgetDefaults::colorPickerStruct::colorPickerStruct() : margin(1, 1, 1, 1), collapsedSize(40, 32), fullSize(250, 180), hueBarPortion(0.18), resultBarPortion(0.35)
	{}

	guiSkinWidgetDefaults::panelStruct::panelStruct() : textFormat(textInit), imageFormat(imageInit), contentPadding(2, 2, 2, 2), baseMargin(1, 1, 1, 1), captionPadding(3, 1, 3, 1), captionHeight(28)
	{
		textFormat.align = textAlignEnum::Center;
	}

	guiSkinWidgetDefaults::spoilerStruct::spoilerStruct() : textFormat(textInit), imageFormat(imageInit), contentPadding(2, 2, 2, 2), baseMargin(1, 1, 1, 1), captionPadding(3, 1, 3, 1), captionHeight(28)
	{
		textFormat.align = textAlignEnum::Center;
	}

	guiSkinWidgetDefaults::scrollbarsStruct::scrollbarsStruct() : scrollbarSize(15), contentPadding(4)
	{}

	guiSkinWidgetDefaults::tooltipStruct::tooltipStruct() : textFormat(textInit)
	{}
}