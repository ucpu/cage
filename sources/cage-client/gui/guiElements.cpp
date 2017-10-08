#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/memory.h>
#include <cage-core/entities.h>
#include <cage-core/utility/png.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/gui.h>
#include <cage-client/graphic.h>
#include "private.h"

namespace cage
{
	namespace
	{
		void initializeTextureUv(elementLayoutStruct &element, const vec2 &outerPos, const vec2 &innerSize, const vec2 &borderSize)
		{
			element.textureUv.defal.outer = vec4(outerPos, outerPos + borderSize + innerSize + borderSize) / 2048;
			element.textureUv.defal.inner = vec4(outerPos + borderSize, outerPos + borderSize + innerSize) / 2048;
			element.textureUv.focus.outer = element.textureUv.defal.outer + 1 * vec4(512, 0, 512, 0) / 2048;
			element.textureUv.focus.inner = element.textureUv.defal.inner + 1 * vec4(512, 0, 512, 0) / 2048;
			element.textureUv.hover.outer = element.textureUv.defal.outer + 2 * vec4(512, 0, 512, 0) / 2048;
			element.textureUv.hover.inner = element.textureUv.defal.inner + 2 * vec4(512, 0, 512, 0) / 2048;
			element.textureUv.disab.outer = element.textureUv.defal.outer + 3 * vec4(512, 0, 512, 0) / 2048;
			element.textureUv.disab.inner = element.textureUv.defal.inner + 3 * vec4(512, 0, 512, 0) / 2048;
		}
	}

	void initializeElements(elementLayoutStruct elements[(uint32)elementTypeEnum::TotalElements])
	{
		for (uint32 i = 0; i < (uint32)elementTypeEnum::TotalElements; i++)
		{
			elementLayoutStruct &el = elements[i];
			el.marginUnits = el.borderUnits = el.paddingUnits = el.defaultSizeUnits = unitsModeEnum::Pixels;
			el.margin = vec4() + 1;
			el.border = vec4() + 4;
			el.padding = vec4(5, 1, 5, 1);
		}

		elements[(uint32)elementTypeEnum::Empty].border = vec4();
		elements[(uint32)elementTypeEnum::Empty].padding = vec4();
		elements[(uint32)elementTypeEnum::Empty].defaultSize = vec2();

		elements[(uint32)elementTypeEnum::Panel].padding = vec4() + 8;

		elements[(uint32)elementTypeEnum::ButtonNormal].defaultSize = vec2(150, 32);

		elements[(uint32)elementTypeEnum::Textbox].defaultSize = vec2(300, 32);
		elements[(uint32)elementTypeEnum::Textbox].border -= 2;
		elements[(uint32)elementTypeEnum::Textbox].padding += 2;

		elements[(uint32)elementTypeEnum::ComboboxBase].defaultSize = vec2(250, 32);
		elements[(uint32)elementTypeEnum::ComboboxList].margin[1] = -6;
		elements[(uint32)elementTypeEnum::ComboboxList].padding *= vec4(0, 1, 0, 1);
		elements[(uint32)elementTypeEnum::ComboboxItem].defaultSize = vec2(215, 22);
		elements[(uint32)elementTypeEnum::ComboboxItem].border = vec4();
		elements[(uint32)elementTypeEnum::ComboboxItem].margin = vec4();

		elements[(uint32)elementTypeEnum::ListboxList].defaultSize = vec2(250, 32);
		elements[(uint32)elementTypeEnum::ListboxItem].defaultSize = vec2(215, 22);
		elements[(uint32)elementTypeEnum::ListboxItem].border = vec4();
		elements[(uint32)elementTypeEnum::ListboxItem].margin = vec4();

		elements[(uint32)elementTypeEnum::CheckboxUnchecked].defaultSize = vec2(28, 28);
		elements[(uint32)elementTypeEnum::CheckboxChecked] = elements[(uint32)elementTypeEnum::CheckboxUnchecked];
		elements[(uint32)elementTypeEnum::CheckboxUndetermined] = elements[(uint32)elementTypeEnum::CheckboxUnchecked];

		elements[(uint32)elementTypeEnum::SliderHorizontalPanel].defaultSize = vec2(200, 28);
		elements[(uint32)elementTypeEnum::SliderHorizontalPanel].padding[0] = elements[(uint32)elementTypeEnum::SliderHorizontalPanel].padding[1];
		elements[(uint32)elementTypeEnum::SliderHorizontalPanel].padding[2] = elements[(uint32)elementTypeEnum::SliderHorizontalPanel].padding[3];
		elements[(uint32)elementTypeEnum::SliderVerticalPanel] = elements[(uint32)elementTypeEnum::SliderHorizontalPanel];

		initializeTextureUv(elements[(uint32)elementTypeEnum::ButtonNormal], /*----------*/ vec2(20 + +000, 20 + +000), vec2(400, 40), vec2(20, 20));
		initializeTextureUv(elements[(uint32)elementTypeEnum::ButtonPressed], /*---------*/ vec2(20 + +000, 20 + +100), vec2(400, 40), vec2(20, 20));
		initializeTextureUv(elements[(uint32)elementTypeEnum::Textbox], /*---------------*/ vec2(20 + +000, 20 + +200), vec2(400, 40), vec2(20, 20));
		initializeTextureUv(elements[(uint32)elementTypeEnum::Textarea], /*--------------*/ vec2(20 + +000, 20 + +300), vec2(400, 40), vec2(20, 20));
		initializeTextureUv(elements[(uint32)elementTypeEnum::ComboboxBase], /*----------*/ vec2(20 + +000, 20 + +400), vec2(400, 40), vec2(20, 20));
		initializeTextureUv(elements[(uint32)elementTypeEnum::ComboboxList], /*----------*/ vec2(20 + +000, 20 + +500), vec2(400, 40), vec2(20, 20));
		initializeTextureUv(elements[(uint32)elementTypeEnum::ComboboxItem], /*----------*/ vec2(20 + +000, 20 + +600), vec2(400, 40), vec2(20, 20));
		initializeTextureUv(elements[(uint32)elementTypeEnum::ListboxList], /*-----------*/ vec2(20 + +000, 20 + +700), vec2(400, 40), vec2(20, 20));
		initializeTextureUv(elements[(uint32)elementTypeEnum::ListboxItem], /*-----------*/ vec2(20 + +000, 20 + +800), vec2(400, 40), vec2(20, 20));
		initializeTextureUv(elements[(uint32)elementTypeEnum::CheckboxUnchecked], /*-----*/ vec2(20 + +000, 20 + +900), vec2(+40, 40), vec2(20, 20));
		initializeTextureUv(elements[(uint32)elementTypeEnum::CheckboxChecked], /*-------*/ vec2(20 + +100, 20 + +900), vec2(+40, 40), vec2(20, 20));
		initializeTextureUv(elements[(uint32)elementTypeEnum::CheckboxUndetermined], /*--*/ vec2(20 + +200, 20 + +900), vec2(+40, 40), vec2(20, 20));
		initializeTextureUv(elements[(uint32)elementTypeEnum::RadioUnchecked], /*--------*/ vec2(20 + +000, 20 + 1000), vec2(+80, 80), vec2(+0, +0));
		initializeTextureUv(elements[(uint32)elementTypeEnum::RadioChecked], /*----------*/ vec2(20 + +100, 20 + 1000), vec2(+80, 80), vec2(+0, +0));
		initializeTextureUv(elements[(uint32)elementTypeEnum::SliderHorizontalPanel], /*-*/ vec2(20 + +000, 20 + 1100), vec2(+40, 40), vec2(20, 20));
		initializeTextureUv(elements[(uint32)elementTypeEnum::SliderVerticalPanel], /*---*/ vec2(20 + +200, 20 + 1100), vec2(+40, 40), vec2(20, 20));
		initializeTextureUv(elements[(uint32)elementTypeEnum::SliderHorizontalDot], /*---*/ vec2(20 + +100, 20 + 1100), vec2(+00, +0), vec2(40, 40));
		initializeTextureUv(elements[(uint32)elementTypeEnum::SliderVerticalDot], /*-----*/ vec2(20 + +300, 20 + 1100), vec2(+00, +0), vec2(40, 40));
		initializeTextureUv(elements[(uint32)elementTypeEnum::Panel], /*-----------------*/ vec2(20 + +000, 1600), vec2(400, 400), vec2(20, 20));
	}

	namespace detail
	{
		namespace
		{
			void renderRectangle(pngImageClass *png, uint32 x1, uint32 y1, uint32 x2, uint32 y2, const vec3 &color)
			{
				uint8 c[3] = { numeric_cast<uint8>(color[0] * 255), numeric_cast<uint8>(color[1] * 255), numeric_cast<uint8>(color[2] * 255) };
				CAGE_ASSERT_RUNTIME(x1 <= x2 && y1 <= y2);
				CAGE_ASSERT_RUNTIME(x2 < png->width() && y2 < png->height());
				uint8 *colors = (uint8*)png->bufferData();
				uint32 w = png->width();
				for (uint32 y = y1; y < y2; y++)
				{
					for (uint32 x = x1; x < x2; x++)
					{
						uint8 *d = colors + (y * w + x) * 4;
						CAGE_ASSERT_RUNTIME(d < colors + png->bufferSize() + 4);
						for (uint32 i = 0; i < 3; i++)
							d[i] = c[i];
						d[3] = 255;
					}
				}
			}

			void renderRectangle(pngImageClass *png, const vec4 &rect, const vec3 &color)
			{
				renderRectangle(png,
					numeric_cast<uint32>(rect[0] * png->width()),
					numeric_cast<uint32>(rect[1] * png->height()),
					numeric_cast<uint32>(rect[2] * png->width()),
					numeric_cast<uint32>(rect[3] * png->height()),
					color);
			}

			void renderRectangle(pngImageClass *png, const elementLayoutStruct::textureUvStruct::textureUvOiStruct &rects, const vec3 &outerBorder, const vec3 &innerBorder, const vec3 &content)
			{
				renderRectangle(png, rects.outer, outerBorder);
				renderRectangle(png, interpolate(rects.outer, rects.inner, 0.5), innerBorder);
				renderRectangle(png, rects.inner, content);
			}
		}

		void guiElementsLayoutTemplateExport(elementLayoutStruct elements[(uint32)elementTypeEnum::TotalElements], uint32 resolution, const string &path)
		{
			holder<pngImageClass> png = newPngImage();
			png->empty(resolution, resolution, 4);
			for (uint32 type = 0; type < (uint32)elementTypeEnum::TotalElements; type++)
			{
				elementLayoutStruct::textureUvStruct &element = elements[type].textureUv;
				renderRectangle(png.get(), element.defal, vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1));
				renderRectangle(png.get(), element.focus, vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1));
				renderRectangle(png.get(), element.hover, vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1));
				renderRectangle(png.get(), element.disab, vec3(.4, .4, .4), vec3(.5, .5, .5), vec3(.6, .6, .6));
			}
			png->encodeFile(path);
		}
	}
}