#include <vector>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/log.h>
#include <cage-core/utility/utf.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphics.h>
#include <cage-client/graphics/shaderConventions.h>
#include <cage-client/opengl.h>
#include <cage-client/assetStructs.h>
#include "private.h"

namespace cage
{
	fontClass::formatStruct::formatStruct() : align(textAlignEnum::Left), size(13), wrapWidth(real::PositiveInfinity), lineSpacing(0)
	{}

	namespace
	{
		struct processDataStruct
		{
			vec3 color;
			vec2 mousePosition;
			vec2 pos;
			mutable vec2 outSize;
			const fontClass::formatStruct *format;
			const uint32 *gls;
			mutable uint32 outCursor;
			uint32 count;
			uint32 cursor;
			bool render;
			processDataStruct() : mousePosition(vec2::Nan), format(nullptr), gls(nullptr),
				outCursor(0), count(0), cursor(-1), render(false)
			{}
		};

		class fontImpl : public fontClass
		{
			static const uint32 charsPerBatch = 256;

		public:
			mutable holder<uniformBufferClass> uni;
			holder<textureClass> tex;
			uint32 texWidth, texHeight;
			shaderClass *shr;
			meshClass *msh;
			uint32 spaceGlyph;
			uint32 returnGlyph;
			uint32 cursorGlyph;
			real lineHeight;
			real firstLineOffset;

			std::vector<fontHeaderStruct::glyphDataStruct> glyphsArray;
			std::vector<real> kerning;
			std::vector<uint32> charmapChars;
			std::vector<uint32> charmapGlyphs;

			fontHeaderStruct::glyphDataStruct getGlyph(uint32 glyphIndex, real size)
			{
				fontHeaderStruct::glyphDataStruct r(glyphsArray[glyphIndex]);
				r.advance *= size;
				r.bearing *= size;
				r.size *= size;
				return r;
			}

			struct InstanceStruct
			{
				vec4 wrld;
				vec4 text;
				InstanceStruct(real x, real y, const fontHeaderStruct::glyphDataStruct &g)
				{
					text = g.texUv;
					wrld[0] = x + g.bearing[0];
					wrld[1] = y + g.bearing[1] - g.size[1];
					wrld[2] = g.size[0];
					wrld[3] = g.size[1];
				}
			};
			mutable std::vector<InstanceStruct> instances;

			fontImpl(windowClass *gl) : texWidth(0), texHeight(0),
				shr(nullptr), msh(nullptr), spaceGlyph(0), returnGlyph(0), cursorGlyph(-1)
			{
				uni = newUniformBuffer(gl);
				uni->writeWhole(nullptr, sizeof(InstanceStruct) * charsPerBatch, GL_DYNAMIC_DRAW);
				tex = newTexture(gl);
				instances.reserve(1000);
			}

			real findKerning(uint32 L, uint32 R, real size) const
			{
				CAGE_ASSERT_RUNTIME(L < glyphsArray.size() && R < glyphsArray.size(), L, R, glyphsArray.size());
				if (kerning.empty() || glyphsArray.empty())
					return 0;
				uint32 s = numeric_cast<uint32>(glyphsArray.size());
				return kerning[L * s + R] * size;
			}

			uint32 findGlyphIndex(uint32 character) const
			{
				CAGE_ASSERT_RUNTIME(charmapChars.size());
				if (charmapChars[0] > character)
					return 0; // otherwise the mid-1 could overflow
				uint32 min = 0, max = numeric_cast<uint32>(charmapChars.size() - 1);
				while (min < max)
				{
					uint32 mid = (max + min) / 2;
					if (charmapChars[mid] > character)
						max = mid - 1;
					else if (charmapChars[mid] < character)
						min = mid + 1;
					else
						min = max = mid;
				}
				if (charmapChars[min] == character)
					return charmapGlyphs[min];
				return 0;
			}

			void processLine(const processDataStruct &data, const uint32 *begin, const uint32 *end, real lineWidth, real lineY)
			{
				real x;
				switch (data.format->align)
				{
				case textAlignEnum::Left: x = data.pos[0]; break;
				case textAlignEnum::Right: x = data.pos[0] + data.format->wrapWidth - lineWidth; break;
				case textAlignEnum::Center: x = data.pos[0] + (data.format->wrapWidth - lineWidth) / 2; break;
				default: CAGE_THROW_CRITICAL(exception, "invalid align enum value");
				}

				vec2 mousePos = data.mousePosition + vec2(-x, lineY + lineHeight * data.format->size);
				bool mouseInLine = mousePos[1] >= 0 && mousePos[1] <= lineHeight * data.format->size + data.format->lineSpacing;
				if (!data.render && !mouseInLine)
					return;
				if (mouseInLine)
				{
					if (mousePos[0] < 0)
						data.outCursor = numeric_cast<uint32>(begin - data.gls);
					else if (mousePos[0] >= lineWidth)
						data.outCursor = numeric_cast<uint32>(end - data.gls);
				}

				uint32 prev = 0;
				while (begin != end)
				{
					if (data.render && begin == data.gls + data.cursor)
					{
						fontHeaderStruct::glyphDataStruct g = getGlyph(cursorGlyph, data.format->size);
						instances.emplace_back(x, lineY, g);
					}
					x += findKerning(prev, *begin, data.format->size);
					prev = *begin++;
					fontHeaderStruct::glyphDataStruct g = getGlyph(prev, data.format->size);
					if (data.render)
						instances.emplace_back(x, lineY, g);
					if (mouseInLine && data.mousePosition[0] >= x && data.mousePosition[0] <= x + g.advance)
						data.outCursor = numeric_cast<uint32>(begin - data.gls);
					x += g.advance;
				}
				if (data.render && begin == data.gls + data.cursor)
				{
					fontHeaderStruct::glyphDataStruct g = getGlyph(cursorGlyph, data.format->size);
					instances.emplace_back(x, lineY, g);
				}
			}

			void processText(const processDataStruct &data)
			{
				CAGE_ASSERT_RUNTIME(data.format->align <= textAlignEnum::Center, data.format->align);
				CAGE_ASSERT_RUNTIME(data.format->wrapWidth > 0, data.format->wrapWidth);
				CAGE_ASSERT_RUNTIME(data.format->size > 0, data.format->size);
				const uint32 *totalEnd = data.gls + data.count;
				const uint32 *lineStart = data.gls;
				real lineY = -data.pos[1] + firstLineOffset * data.format->size;
				while (true)
				{
					const uint32 *lineEnd = lineStart;
					const uint32 *wrap = lineStart;
					real lineWidth = 0;
					real wrapWidth = 0;

					while (lineEnd != totalEnd && *lineEnd != returnGlyph)
					{
						real k = findKerning(lineEnd != lineStart ? lineEnd[-1] : 0, *lineEnd, data.format->size);
						real a = getGlyph(*lineEnd, data.format->size).advance;
						if (lineWidth + k + a > data.format->wrapWidth && lineWidth > 0)
						{
							if (wrap != lineStart)
							{
								lineEnd = wrap;
								lineWidth = wrapWidth;
							}
							break;
						}
						else if (*lineEnd == spaceGlyph)
						{
							wrap = lineEnd;
							wrapWidth = lineWidth;
						}
						lineEnd++;
						lineWidth += k + a;
					}

					processLine(data, lineStart, lineEnd, lineWidth, lineY);
					data.outSize[0] = max(data.outSize[0], lineWidth);
					data.outSize[1] += (lineHeight + data.format->lineSpacing) * data.format->size;
					lineStart = lineEnd;
					if (lineStart == totalEnd)
						break;
					if (*lineStart == returnGlyph || *lineStart == spaceGlyph)
						lineStart++;
					lineY -= (lineHeight + data.format->lineSpacing) * data.format->size;
				}

				if (data.render)
				{
					shr->uniform(2, data.color);

					uint32 s = numeric_cast<uint32>(instances.size());
					uint32 a = s / charsPerBatch;
					uint32 b = s - a * charsPerBatch;
					for (uint32 i = 0; i < a; i++)
					{
						uni->bind();
						uni->writeRange(&instances[i * charsPerBatch], 0, sizeof(InstanceStruct) * charsPerBatch);
						uni->bind(1);
						msh->dispatch(charsPerBatch);
					}
					if (b)
					{
						uni->bind();
						uni->writeRange(&instances[a * charsPerBatch], 0, sizeof(InstanceStruct) * b);
						uni->bind(1);
						msh->dispatch(b);
					}
					instances.clear();
				}
			}
		};
	}

	void fontClass::bind(meshClass *mesh, shaderClass *shader, uint32 screenWidth, uint32 screenHeight) const
	{
		fontImpl *impl = (fontImpl*)this;
		glActiveTexture(GL_TEXTURE0);
		impl->tex->bind();
		impl->msh = mesh;
		impl->msh->bind();
		impl->shr = shader;
		impl->shr->bind();
		impl->shr->uniform(0, vec2(2.0 / screenWidth, 2.0 / screenHeight));
	}

	void fontClass::setLine(real lineHeight, real firstLineOffset)
	{
		fontImpl *impl = (fontImpl*)this;
		impl->lineHeight = lineHeight;
		impl->firstLineOffset = -firstLineOffset;
	}

	void fontClass::setImage(uint32 width, uint32 height, uint32 size, const void *data)
	{
		fontImpl *impl = (fontImpl*)this;
		impl->texWidth = width;
		impl->texHeight = height;
		impl->tex->bind();
		uint32 bpp = size / (width * height);
		CAGE_ASSERT_RUNTIME(width * height * bpp == size, "rounding error", width, height, bpp, size);
		switch (bpp)
		{
		case 1:
			impl->tex->image2d(width, height, GL_R8, GL_RED, GL_UNSIGNED_BYTE, data);
			break;
		case 2:
			impl->tex->image2d(width, height, GL_R16, GL_RED, GL_UNSIGNED_SHORT, data);
			break;
		case 3:
			impl->tex->image2d(width, height, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, data);
			break;
		case 4:
			impl->tex->image2d(width, height, GL_R32F, GL_RED, GL_FLOAT, data);
			break;
		case 6:
			impl->tex->image2d(width, height, GL_RGB16, GL_RGB, GL_UNSIGNED_SHORT, data);
			break;
		case 12:
			impl->tex->image2d(width, height, GL_RGB32F, GL_RGB, GL_FLOAT, data);
			break;
		default:
			CAGE_THROW_ERROR(exception, "unsupported bpp");
		}
		impl->tex->filters(GL_LINEAR, GL_LINEAR, 100);
		impl->tex->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		//impl->tex->generateMipmaps();
	}

	void fontClass::setGlyphs(uint32 count, const void *data, const real *kerning)
	{
		fontImpl *impl = (fontImpl*)this;
		impl->glyphsArray.resize(count);
		detail::memcpy(impl->glyphsArray.data(), data, sizeof(fontHeaderStruct::glyphDataStruct) * count);
		if (kerning)
		{
			impl->kerning.resize(count * count);
			detail::memcpy(impl->kerning.data(), kerning, count * count * sizeof(real));
		}
		else
			impl->kerning.clear();
		impl->cursorGlyph = count - 1; // last glyph
	}

	void fontClass::setCharmap(uint32 count, const uint32 *chars, const uint32 *glyphs)
	{
		fontImpl *impl = (fontImpl*)this;
		impl->charmapChars.resize(count);
		impl->charmapGlyphs.resize(count);
		if (count > 0)
		{
			detail::memcpy(impl->charmapChars.data(), chars, sizeof(uint32) * count);
			detail::memcpy(impl->charmapGlyphs.data(), glyphs, sizeof(uint32) * count);
		}
		impl->returnGlyph = impl->findGlyphIndex('\n');
		impl->spaceGlyph = impl->findGlyphIndex(' ');
	}

	void fontClass::transcript(const string &text, uint32 *glyphs, uint32 &count)
	{
		transcript(text.c_str(), glyphs, count);
	}

	void fontClass::transcript(const char *text, uint32 *glyphs, uint32 &count)
	{
		fontImpl *impl = (fontImpl*)this;
		if (glyphs)
		{
			convert8to32(glyphs, count, text);
			for (uint32 i = 0; i < count; i++)
				glyphs[i] = impl->findGlyphIndex(glyphs[i]);
		}
		else
			count = countCharacters(text);
	}

	void fontClass::size(const uint32 *glyphs, uint32 count, const formatStruct &format, vec2 &size)
	{
		processDataStruct data;
		data.format = &format;
		data.gls = glyphs;
		data.count = count;
		((fontImpl*)this)->processText(data);
		size = data.outSize;
	}

	void fontClass::size(const uint32 *glyphs, uint32 count, const formatStruct &format, vec2 &size, const vec2 &mousePosition, uint32 &cursor)
	{
		processDataStruct data;
		data.mousePosition = mousePosition;
		data.format = &format;
		data.gls = glyphs;
		data.count = count;
		data.outCursor = cursor;
		((fontImpl*)this)->processText(data);
		size = data.outSize;
		cursor = data.outCursor;
	}

	void fontClass::render(const uint32 *glyphs, uint32 count, const formatStruct &format, const vec2 &position, const vec3 &color, uint32 cursor)
	{
		processDataStruct data;
		data.format = &format;
		data.gls = glyphs;
		data.count = count;
		data.cursor = getApplicationTime() % 1000000 < 300000 ? -1 : cursor;
		data.pos = position;
		data.render = true;
		data.color = color;
		((fontImpl*)this)->processText(data);
	}

	holder<fontClass> newFont(windowClass *context)
	{
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentContext() == context);
		return detail::systemArena().createImpl<fontClass, fontImpl>(context);
	}
}
