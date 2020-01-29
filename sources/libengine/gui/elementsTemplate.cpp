#include <cage-core/image.h>

#include "private.h"

namespace cage
{
	namespace detail
	{
		namespace
		{
			void renderRectangle(Image *png, uint32 x1, uint32 y1, uint32 x2, uint32 y2, const vec3 &color)
			{
				uint8 c[3] = { numeric_cast<uint8>(color[0] * 255), numeric_cast<uint8>(color[1] * 255), numeric_cast<uint8>(color[2] * 255) };
				CAGE_ASSERT(x1 <= x2 && y1 <= y2);
				CAGE_ASSERT(x2 < png->width() && y2 < png->height());
				uint8 *colors = (uint8*)png->bufferData();
				uint32 w = png->width();
				for (uint32 y = y1; y < y2; y++)
				{
					for (uint32 x = x1; x < x2; x++)
					{
						uint8 *d = colors + (y * w + x) * 4;
						CAGE_ASSERT(d + 4 <= colors + png->bufferSize());
						for (uint32 i = 0; i < 3; i++)
							d[i] = c[i];
						d[3] = 255;
					}
				}
			}

			void renderRectangle(Image *png, const vec4 &rect, const vec3 &color)
			{
				renderRectangle(png,
					numeric_cast<uint32>(rect[0] * png->width()),
					numeric_cast<uint32>(rect[1] * png->height()),
					numeric_cast<uint32>(rect[2] * png->width()),
					numeric_cast<uint32>(rect[3] * png->height()),
					color);
			}

			void renderRectangle(Image *png, const GuiSkinElementLayout::TextureUvOi &rects, const vec3 &outerBorder, const vec3 &innerBorder, const vec3 &content)
			{
				renderRectangle(png, rects.outer, outerBorder);
				renderRectangle(png, interpolate(rects.outer, rects.inner, 0.5), innerBorder);
				renderRectangle(png, rects.inner, content);
			}
		}

		Holder<Image> guiSkinTemplateExport(const GuiSkinConfig &skin, uint32 resolution)
		{
			Holder<Image> png = newImage();
			png->empty(resolution, resolution, 4);
			for (uint32 type = 0; type < (uint32)GuiElementTypeEnum::TotalElements; type++)
			{
				const GuiSkinElementLayout::TextureUv &element = skin.layouts[type].textureUv;
				renderRectangle(png.get(), element.data[3], vec3(.4, .4, .4), vec3(.5, .5, .5), vec3(.6, .6, .6));
				renderRectangle(png.get(), element.data[2], vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1));
				renderRectangle(png.get(), element.data[1], vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 0.66));
				renderRectangle(png.get(), element.data[0], vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 0.33));
			}
			return png;
		}
	}
}
