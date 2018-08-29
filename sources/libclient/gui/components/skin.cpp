#include <set>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/utility/hashString.h>

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
		uint32 font;
		textFormatComponent text;
		imageFormatComponent image;

		struct skinInitializerStruct
		{
			skinInitializerStruct()
			{
				font = hashString("cage/font/ubuntu/Ubuntu-R.ttf");
				text.color = vec3();
				text.fontName = font;
				text.align = textAlignEnum::Left;
				text.lineSpacing = 0;
				image.animationOffset = 0;
				image.animationSpeed = 1;
				image.mode = imageModeEnum::Stretch;
			}
		} skinInitializer;
	}

	skinElementLayoutStruct::textureUvStruct::textureUvStruct()
	{
		detail::memset(this, 0, sizeof(*this));
	}

	skinElementLayoutStruct::skinElementLayoutStruct() : border(5, 5, 5, 5)
	{}

	skinWidgetDefaultsStruct::skinWidgetDefaultsStruct()
	{}

	namespace
	{
		struct packerStruct
		{
			vec2 size; // including border
			real frame; // spacing around element
			real border; // border inside element

			skinElementLayoutStruct::textureUvOiStruct next(bool allowBorder)
			{
				vec2 s = size + frame * 2;
				CAGE_ASSERT_RUNTIME(s[0] < 1, s, frame, border, size);
				if (p[0] + s[0] > 1)
					newLine();
				skinElementLayoutStruct::textureUvOiStruct r;
				r.outer[0] = p[0] + frame;
				r.outer[1] = p[1] + frame;
				r.outer[2] = r.outer[0] + size[0];
				r.outer[3] = r.outer[1] + size[1];
				r.inner = r.outer;
				if (allowBorder)
				{
					CAGE_ASSERT_RUNTIME(size[0] >= 2 * border && size[1] >= 2 * border, frame, border, size);
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

	skinConfigStruct::skinConfigStruct() : textureName(hashString("cage/texture/gui.psd"))
	{
		std::vector<elementTypeEnum> largeElements = {
			elementTypeEnum::WindowBaseNormal,
			elementTypeEnum::GroupPanel,
			elementTypeEnum::TextArea,
			elementTypeEnum::ComboBoxList,
			elementTypeEnum::ListBoxList,
		};

		std::vector<elementTypeEnum> wideElements = {
			elementTypeEnum::Button,
			elementTypeEnum::ButtonTop,
			elementTypeEnum::ButtonBottom,
			elementTypeEnum::ButtonLeft,
			elementTypeEnum::ButtonRight,
			elementTypeEnum::ButtonHorizontal,
			elementTypeEnum::ButtonVertical,
			elementTypeEnum::InputBox,
			elementTypeEnum::ComboBoxBase,
			elementTypeEnum::ComboBoxItem,
			elementTypeEnum::ListBoxItem,
			elementTypeEnum::ProgressBar,
			elementTypeEnum::WindowBaseModal,
			elementTypeEnum::WindowCaption,
			elementTypeEnum::GroupCaption,
			elementTypeEnum::TaskBarBase,
			elementTypeEnum::ToolTip,
		};

		std::vector<elementTypeEnum> smallElements = {
			elementTypeEnum::ScrollbarHorizontalPanel,
			elementTypeEnum::ScrollbarVerticalPanel,
			elementTypeEnum::ScrollbarHorizontalDot,
			elementTypeEnum::ScrollbarVerticalDot,
			elementTypeEnum::SliderHorizontalPanel,
			elementTypeEnum::SliderVerticalPanel,
			elementTypeEnum::SliderHorizontalDot,
			elementTypeEnum::SliderVerticalDot,
			elementTypeEnum::GroupCell,
			elementTypeEnum::GroupSpoilerCollapsed,
			elementTypeEnum::GroupSpoilerShown,
			elementTypeEnum::InputButtonDecrement,
			elementTypeEnum::InputButtonIncrement,
			elementTypeEnum::CheckBoxUnchecked,
			elementTypeEnum::CheckBoxChecked,
			elementTypeEnum::CheckBoxIndetermined,
			elementTypeEnum::RadioBoxUnchecked,
			elementTypeEnum::RadioBoxChecked,
			elementTypeEnum::ColorPickerCompact,
			elementTypeEnum::ColorPickerResult,
			elementTypeEnum::WindowButtonMinimize,
			elementTypeEnum::WindowButtonMaximize,
			elementTypeEnum::WindowButtonRestore,
			elementTypeEnum::WindowButtonClose,
			elementTypeEnum::WindowResizer,
			elementTypeEnum::TaskBarItem,
		};

		std::set<elementTypeEnum> noBorder = {
			elementTypeEnum::RadioBoxChecked,
			elementTypeEnum::RadioBoxUnchecked,
			elementTypeEnum::SliderHorizontalDot,
			elementTypeEnum::SliderVerticalDot,
			elementTypeEnum::WindowResizer,
		};

		std::set<elementTypeEnum> noFocus = {
			elementTypeEnum::GroupCell, elementTypeEnum::GroupPanel, elementTypeEnum::GroupSpoilerCollapsed, elementTypeEnum::GroupSpoilerShown, elementTypeEnum::GroupCaption,
			elementTypeEnum::WindowBaseModal, elementTypeEnum::WindowBaseNormal, elementTypeEnum::WindowCaption, elementTypeEnum::WindowResizer,
			elementTypeEnum::InputButtonDecrement, elementTypeEnum::InputButtonIncrement,
			elementTypeEnum::ComboBoxList,
			elementTypeEnum::ProgressBar,
			elementTypeEnum::ScrollbarHorizontalDot, elementTypeEnum::ScrollbarHorizontalPanel, elementTypeEnum::ScrollbarVerticalDot, elementTypeEnum::ScrollbarVerticalPanel,
			elementTypeEnum::ColorPickerHSliderPanel, elementTypeEnum::ColorPickerResult, elementTypeEnum::ColorPickerSVRect,
			elementTypeEnum::ToolTip,
		};

		std::set<elementTypeEnum> noHover = {
			elementTypeEnum::GroupCell, elementTypeEnum::GroupPanel,
			elementTypeEnum::WindowBaseModal, elementTypeEnum::WindowBaseNormal,
			elementTypeEnum::ComboBoxList, elementTypeEnum::ListBoxList,
			elementTypeEnum::ProgressBar,
			elementTypeEnum::ColorPickerHSliderPanel, elementTypeEnum::ColorPickerResult, elementTypeEnum::ColorPickerSVRect,
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
				bool border = noBorder.count(it) == 0;
				layouts[(uint32)it].textureUv.data[0] = packer.next(border);
				if (noFocus.count(it) == 0)
					layouts[(uint32)it].textureUv.data[1] = packer.next(border);
				if (noHover.count(it) == 0)
					layouts[(uint32)it].textureUv.data[2] = packer.next(border);
				layouts[(uint32)it].textureUv.data[3] = packer.next(border);
			}
			packer.newLine();

			// wide
			packer.size = vec2(5, 1) * 24.f / 1024;
			for (auto it : wideElements)
			{
				bool border = noBorder.count(it) == 0;
				layouts[(uint32)it].textureUv.data[0] = packer.next(border);
				if (noFocus.count(it) == 0)
					layouts[(uint32)it].textureUv.data[1] = packer.next(border);
				if (noHover.count(it) == 0)
					layouts[(uint32)it].textureUv.data[2] = packer.next(border);
				layouts[(uint32)it].textureUv.data[3] = packer.next(border);
			}
			packer.newLine();

			// small
			packer.size = vec2(1, 1) * 24.f / 1024;
			for (auto it : smallElements)
			{
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

		{ // color picker
			layouts[(uint32)elementTypeEnum::ColorPickerFull].textureUv = layouts[(uint32)elementTypeEnum::ColorPickerCompact].textureUv;
			layouts[(uint32)elementTypeEnum::ColorPickerHSliderPanel].textureUv = layouts[(uint32)elementTypeEnum::ColorPickerResult].textureUv;
			layouts[(uint32)elementTypeEnum::ColorPickerSVRect].textureUv = layouts[(uint32)elementTypeEnum::ColorPickerResult].textureUv;
		}
	}

	skinWidgetDefaultsStruct::labelStruct::labelStruct() : textFormat(text), imageFormat(image), margin(1, 2, 1, 2)
	{}

	skinWidgetDefaultsStruct::buttonStruct::buttonStruct() : textFormat(text), imageFormat(image), padding(1, 1, 1, 1), margin(1, 1, 1, 1), size(150, 32)
	{
		textFormat.align = textAlignEnum::Center;
	}

	skinWidgetDefaultsStruct::inputBoxStruct::inputBoxStruct() : textValidFormat(text), textInvalidFormat(text), placeholderFormat(text), basePadding(2, 2, 2, 2), margin(1, 1, 1, 1), size(300, 32), buttonsSize(32), buttonsOffset(2), buttonsMode(inputButtonsPlacementModeEnum::Sides)
	{
		textInvalidFormat.color = vec3(1,0,0);
		placeholderFormat.color = vec3(0.5,0.5,0.5);
	}

	skinWidgetDefaultsStruct::textAreaStruct::textAreaStruct() : textFormat(text), padding(3, 3, 3, 3), margin(1, 1, 1, 1), size(450, 200)
	{}

	skinWidgetDefaultsStruct::checkBoxStruct::checkBoxStruct() : textFormat(text), margin(1, 1, 1, 1), size(28, 28), labelOffset(3, 5)
	{}

	skinWidgetDefaultsStruct::comboBoxStruct::comboBoxStruct() : placeholderFormat(text), itemsFormat(text), selectedFormat(text), basePadding(1, 1, 1, 1), baseMargin(1, 1, 1, 1), listPadding(0, 0, 0, 0), itemPadding(1, 1, 1, 1), size(250, 32), listOffset(-6), itemSpacing(0)
	{
		placeholderFormat.color = vec3(0.5, 0.5, 0.5);
		placeholderFormat.align = textAlignEnum::Center;
		itemsFormat.align = textAlignEnum::Center;
		selectedFormat.align = textAlignEnum::Center;
	}

	skinWidgetDefaultsStruct::listBoxStruct::listBoxStruct() : textFormat(text), basePadding(0, 0, 0, 0), baseMargin(1, 1, 1, 1), itemPadding(1, 1, 1, 1), size(250, 32), itemSpacing(0)
	{}

	skinWidgetDefaultsStruct::progressBarStruct::progressBarStruct() : textFormat(text), backgroundImageFormat(image), fillingImageFormat(image), baseMargin(1, 1, 1, 1), textPadding(1, 1, 1, 1), fillingPadding(1, 1, 1, 1), size(200, 28)
	{
		fillingImage.animationStart = 0;
		fillingImage.textureName = hashString("todo");
		fillingImage.textureUvOffset = vec2();
		fillingImage.textureUvSize = vec2(1, 1);
	}

	skinWidgetDefaultsStruct::sliderBarStruct::sliderBarStruct()
	{
		horizontal.padding = vec4(1, 1, 1, 1);
		horizontal.margin = vec4(1, 1, 1, 1);
		horizontal.size = vec2(150, 28);
		vertical.padding = vec4(1, 1, 1, 1);
		vertical.margin = vec4(1, 1, 1, 1);
		vertical.size = vec2(28, 150);
	}

	skinWidgetDefaultsStruct::colorPickerStruct::colorPickerStruct() : margin(1, 1, 1, 1), collapsedSize(40, 32), fullSize(250, 180), hueBarPortion(0.18), resultBarPortion(0.35)
	{}

	skinWidgetDefaultsStruct::graphCanvasStruct::graphCanvasStruct() : size(200, 120)
	{}

	skinWidgetDefaultsStruct::scrollableBaseStruct::scrollableBaseStruct() : textFormat(text), contentPadding(2, 2, 2, 2), baseMargin(1, 1, 1, 1), captionPadding(3, 1, 3, 1), scrollbarsSizes(16, 16), captionHeight(28)
	{
		textFormat.align = textAlignEnum::Center;
	}

	skinWidgetDefaultsStruct::groupBoxStruct::groupBoxStruct() :  imageFormat(image)
	{
		text.align = textAlignEnum::Center;
	}

	skinWidgetDefaultsStruct::windowStruct::windowStruct() : imageFormat(image), buttonsSpacing(0)
	{
		textFormat.align = textAlignEnum::Center;
	}

	skinWidgetDefaultsStruct::taskBarStruct::taskBarStruct() : textFormat(text), imageFormat(image), size(400, 32)
	{}

	skinWidgetDefaultsStruct::tooltipStruct::tooltipStruct() : textFormat(text)
	{}
}
