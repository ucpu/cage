#include <vector>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/log.h>
#include <cage-core/utf.h>

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
			vec2 mousePosition;
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
		public:
			holder<uniformBufferClass> uni;
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
			std::vector<InstanceStruct> instances;

			fontImpl(windowClass *gl) : texWidth(0), texHeight(0),
				shr(nullptr), msh(nullptr), spaceGlyph(0), returnGlyph(0), cursorGlyph(-1)
			{
				uni = newUniformBuffer(gl);
				uni->writeWhole(nullptr, sizeof(InstanceStruct) * CAGE_SHADER_MAX_CHARACTERS, GL_DYNAMIC_DRAW);
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

			void processCursor(const processDataStruct &data, const uint32 *begin, real x, real lineY)
			{
				if (data.render && begin == data.gls + data.cursor)
				{
					fontHeaderStruct::glyphDataStruct g = getGlyph(cursorGlyph, data.format->size);
					instances.emplace_back(x, lineY, g);
				}
			}

			void processLine(const processDataStruct &data, const uint32 *begin, const uint32 *end, real lineWidth, real lineY)
			{
				real x;
				switch (data.format->align)
				{
				case textAlignEnum::Left: break;
				case textAlignEnum::Right: x = data.format->wrapWidth - lineWidth; break;
				case textAlignEnum::Center: x = (data.format->wrapWidth - lineWidth) / 2; break;
				default: CAGE_THROW_CRITICAL(exception, "invalid align enum value");
				}

				vec2 mousePos = data.mousePosition + vec2(-x, lineY + lineHeight * data.format->size);
				bool mouseInLine = mousePos[1] >= 0 && mousePos[1] <= (lineHeight + data.format->lineSpacing) * data.format->size;
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
					processCursor(data, begin, x, lineY);
					fontHeaderStruct::glyphDataStruct g = getGlyph(*begin, data.format->size);
					real k = findKerning(prev, *begin, data.format->size);
					prev = *begin++;
					if (data.render)
						instances.emplace_back(x + k, lineY, g);
					if (mouseInLine && data.mousePosition[0] >= x && data.mousePosition[0] < x + k + g.advance)
						data.outCursor = numeric_cast<uint32>(begin - data.gls);
					x += k + g.advance;
				}
				processCursor(data, begin, x, lineY);
			}

			void processText(const processDataStruct &data)
			{
				CAGE_ASSERT_RUNTIME(!data.render || instances.empty());
				CAGE_ASSERT_RUNTIME(data.format->align <= textAlignEnum::Center, data.format->align);
				CAGE_ASSERT_RUNTIME(data.format->wrapWidth > 0, data.format->wrapWidth);
				CAGE_ASSERT_RUNTIME(data.format->size > 0, data.format->size);
				const uint32 *totalEnd = data.gls + data.count;
				const uint32 *it = data.gls;
				real actualLineHeight = (lineHeight + data.format->lineSpacing) * data.format->size;
				real lineY = (firstLineOffset - data.format->lineSpacing * 0.75) * data.format->size;

				if (data.count == 0)
				{ // process cursor
					processLine(data, it, totalEnd, 0, lineY);
				}

				while (it != totalEnd)
				{
					const uint32 *const lineStart = it;
					const uint32 *lineEnd = it;
					real lineWidth = 0;
					real itWidth = 0;

					while (true)
					{
						if (it == totalEnd)
						{
							lineEnd = it;
							lineWidth = itWidth;
							break;
						}
						if (*it == returnGlyph)
						{
							lineEnd = it;
							lineWidth = itWidth;
							it++;
							break;
						}
						real w = getGlyph(*it, data.format->size).advance;
						w += findKerning(it == lineStart ? 0 : it[-1], *it, data.format->size);
						if (it != lineStart && itWidth + w > data.format->wrapWidth + data.format->size * 1e-6)
						{ // at this point, the line needs to be wrapped somewhere
							if (*lineEnd == spaceGlyph)
							{ // if the line has had a space already, use the space for the wrapping point
								it = lineEnd + 1;
							}
							else
							{ // otherwise wrap right now
								lineEnd = it;
								lineWidth = itWidth;
							}
							break;
						}
						if (*it == spaceGlyph)
						{ // remember this position as potential wrapping point
							lineEnd = it;
							lineWidth = itWidth;
						}
						it++;
						itWidth += w;
					}

					processLine(data, lineStart, lineEnd, lineWidth, lineY);
					data.outSize[0] = max(data.outSize[0], lineWidth);
					data.outSize[1] += actualLineHeight;
					lineY -= actualLineHeight;
				}

				if (data.render)
				{
					uni->bind();
					uni->bind(1);

					uint32 s = numeric_cast<uint32>(instances.size());
					uint32 a = s / CAGE_SHADER_MAX_CHARACTERS;
					uint32 b = s - a * CAGE_SHADER_MAX_CHARACTERS;
					for (uint32 i = 0; i < a; i++)
					{
						uni->writeRange(&instances[i * CAGE_SHADER_MAX_CHARACTERS], 0, sizeof(InstanceStruct) * CAGE_SHADER_MAX_CHARACTERS);
						msh->dispatch(CAGE_SHADER_MAX_CHARACTERS);
					}
					if (b)
					{
						uni->writeRange(&instances[a * CAGE_SHADER_MAX_CHARACTERS], 0, sizeof(InstanceStruct) * b);
						msh->dispatch(b);
					}

					instances.clear();
				}
			}
		};
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
		vec2 mp;
		uint32 c;
		this->size(glyphs, count, format, size, mp, c);
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

	void fontClass::bind(meshClass *mesh, shaderClass *shader) const
	{
		fontImpl *impl = (fontImpl*)this;
		glActiveTexture(GL_TEXTURE0);
		impl->tex->bind();
		impl->msh = mesh;
		impl->msh->bind();
		impl->shr = shader;
		impl->shr->bind();
	}

	void fontClass::render(const uint32 *glyphs, uint32 count, const formatStruct &format, uint32 cursor)
	{
		processDataStruct data;
		data.format = &format;
		data.gls = glyphs;
		data.count = count;
		data.cursor = getApplicationTime() % 1000000 < 300000 ? -1 : cursor;
		data.render = true;
		((fontImpl*)this)->processText(data);
	}

	holder<fontClass> newFont(windowClass *context)
	{
		CAGE_ASSERT_RUNTIME(graphicsPrivat::getCurrentContext() == context);
		return detail::systemArena().createImpl<fontClass, fontImpl>(context);
	}
}
