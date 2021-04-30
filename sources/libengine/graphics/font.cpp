#include <cage-core/geometry.h>
#include <cage-core/utf.h>
#include <cage-core/serialization.h>

#include <cage-engine/shaderConventions.h>
#include <cage-engine/assetStructs.h>
#include <cage-engine/opengl.h>
#include <cage-engine/font.h>
#include <cage-engine/texture.h>
#include <cage-engine/uniformBuffer.h>
#include <cage-engine/model.h>
#include <cage-engine/shaderProgram.h>
#include "private.h"

#include <vector>
#include <cstring>

namespace cage
{
	namespace
	{
		struct ProcessData
		{
			vec2 mousePosition;
			mutable vec2 outSize;
			const Font::FormatStruct *format;
			const uint32 *gls;
			mutable uint32 outCursor;
			uint32 count;
			uint32 cursor;
			bool render;
			ProcessData() : mousePosition(vec2::Nan()), format(nullptr), gls(nullptr),
				outCursor(0), count(0), cursor(m), render(false)
			{}
		};

		class FontImpl : public Font
		{
		public:
			Holder<Texture> tex;
			uint32 texWidth, texHeight;
			ShaderProgram *shr;
			Model *msh;
			uint32 spaceGlyph;
			uint32 returnGlyph;
			uint32 cursorGlyph;
			real lineHeight;
			real firstLineOffset;

			std::vector<FontHeader::GlyphData> glyphsArray;
			std::vector<real> kerning;
			std::vector<uint32> charmapChars;
			std::vector<uint32> charmapGlyphs;

			FontHeader::GlyphData getGlyph(uint32 glyphIndex, real size)
			{
				FontHeader::GlyphData r(glyphsArray[glyphIndex]);
				r.advance *= size;
				r.bearing *= size;
				r.size *= size;
				return r;
			}

			struct Instance
			{
				vec4 wrld;
				vec4 text;
				Instance(real x, real y, const FontHeader::GlyphData &g)
				{
					text = g.texUv;
					wrld[0] = x + g.bearing[0];
					wrld[1] = y + g.bearing[1] - g.size[1];
					wrld[2] = g.size[0];
					wrld[3] = g.size[1];
				}
			};
			std::vector<Instance> instances;

			FontImpl() : texWidth(0), texHeight(0),
				shr(nullptr), msh(nullptr), spaceGlyph(0), returnGlyph(0), cursorGlyph(m)
			{
				tex = newTexture();
				instances.reserve(1000);
			}

			real findKerning(uint32 L, uint32 R, real size) const
			{
				CAGE_ASSERT(L < glyphsArray.size() && R < glyphsArray.size());
				if (kerning.empty() || glyphsArray.empty())
					return 0;
				uint32 s = numeric_cast<uint32>(glyphsArray.size());
				return kerning[L * s + R] * size;
			}

			uint32 findGlyphIndex(uint32 character) const
			{
				CAGE_ASSERT(charmapChars.size());
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

			void processCursor(const ProcessData &data, const uint32 *begin, real x, real lineY)
			{
				if (data.render && begin == data.gls + data.cursor)
				{
					FontHeader::GlyphData g = getGlyph(cursorGlyph, data.format->size);
					instances.emplace_back(x, lineY, g);
				}
			}

			void processLine(const ProcessData &data, const uint32 *begin, const uint32 *end, real lineWidth, real lineY)
			{
				real x;
				switch (data.format->align)
				{
				case TextAlignEnum::Left: break;
				case TextAlignEnum::Right: x = data.format->wrapWidth - lineWidth; break;
				case TextAlignEnum::Center: x = (data.format->wrapWidth - lineWidth) / 2; break;
				default: CAGE_THROW_CRITICAL(Exception, "invalid align enum value");
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
					FontHeader::GlyphData g = getGlyph(*begin, data.format->size);
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

			void processText(const ProcessData &data)
			{
				CAGE_ASSERT(!data.render || instances.empty());
				CAGE_ASSERT(data.format->align <= TextAlignEnum::Center);
				CAGE_ASSERT(data.format->wrapWidth > 0);
				CAGE_ASSERT(data.format->size > 0);
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
					uint32 s = numeric_cast<uint32>(instances.size());
					uint32 a = s / CAGE_SHADER_MAX_CHARACTERS;
					uint32 b = s - a * CAGE_SHADER_MAX_CHARACTERS;
					for (uint32 i = 0; i < a; i++)
					{
						Holder<UniformBuffer> uni = newUniformBuffer();
						uni->bind(1);
						auto p = instances.data() + i * CAGE_SHADER_MAX_CHARACTERS;
						PointerRange<Instance> r = { p, p + sizeof(Instance) * CAGE_SHADER_MAX_CHARACTERS };
						uni->writeWhole(bufferCast(r), GL_STREAM_DRAW);
						msh->dispatch(CAGE_SHADER_MAX_CHARACTERS);
					}
					if (b)
					{
						Holder<UniformBuffer> uni = newUniformBuffer();
						uni->bind(1);
						auto p = instances.data() + a * CAGE_SHADER_MAX_CHARACTERS;
						PointerRange<Instance> r = { p, p + b * sizeof(Instance) };
						uni->writeWhole(bufferCast(r), GL_STREAM_DRAW);
						msh->dispatch(b);
					}

					instances.clear();
				}
			}
		};
	}

	void Font::setDebugName(const string &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
		FontImpl *impl = (FontImpl *)this;
		impl->tex->setDebugName(name);
	}

	void Font::setLine(real lineHeight, real firstLineOffset)
	{
		FontImpl *impl = (FontImpl *)this;
		impl->lineHeight = lineHeight;
		impl->firstLineOffset = -firstLineOffset;
	}

	void Font::setImage(uint32 width, uint32 height, PointerRange<const char> buffer)
	{
		FontImpl *impl = (FontImpl *)this;
		impl->texWidth = width;
		impl->texHeight = height;
		impl->tex->bind();
		const uint32 channels = numeric_cast<uint32>(buffer.size() / (width * height));
		CAGE_ASSERT(width * height * channels == buffer.size());
		switch (channels)
		{
		case 1:
			impl->tex->image2d(width, height, GL_R8, GL_RED, GL_UNSIGNED_BYTE, buffer);
			break;
		case 2:
			impl->tex->image2d(width, height, GL_R16, GL_RED, GL_UNSIGNED_SHORT, buffer);
			break;
		case 3:
			impl->tex->image2d(width, height, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, buffer);
			break;
		case 4:
			impl->tex->image2d(width, height, GL_R32F, GL_RED, GL_FLOAT, buffer);
			break;
		case 6:
			impl->tex->image2d(width, height, GL_RGB16, GL_RGB, GL_UNSIGNED_SHORT, buffer);
			break;
		case 12:
			impl->tex->image2d(width, height, GL_RGB32F, GL_RGB, GL_FLOAT, buffer);
			break;
		default:
			CAGE_THROW_ERROR(Exception, "font: unsupported image bpp");
		}
		impl->tex->filters(GL_LINEAR, GL_LINEAR, 0);
		impl->tex->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		//impl->tex->generateMipmaps();
	}

	void Font::setGlyphs(PointerRange<const char> buffer, PointerRange<const real> kerning)
	{
		FontImpl *impl = (FontImpl *)this;
		const uint32 count = numeric_cast<uint32>(buffer.size() / sizeof(FontHeader::GlyphData));
		CAGE_ASSERT(buffer.size() == count * sizeof(FontHeader::GlyphData));
		impl->glyphsArray.resize(count);
		detail::memcpy(impl->glyphsArray.data(), buffer.data(), buffer.size());
		if (!kerning.empty())
		{
			CAGE_ASSERT(kerning.size() == count * count);
			impl->kerning.resize(count * count);
			detail::memcpy(impl->kerning.data(), kerning.data(), kerning.size() * sizeof(real));
		}
		else
			impl->kerning.clear();
		impl->cursorGlyph = count - 1; // last glyph
	}

	void Font::setCharmap(PointerRange<const uint32> chars, PointerRange<const uint32> glyphs)
	{
		FontImpl *impl = (FontImpl *)this;
		CAGE_ASSERT(chars.size() == glyphs.size());
		impl->charmapChars.resize(chars.size());
		impl->charmapGlyphs.resize(chars.size());
		detail::memcpy(impl->charmapChars.data(), chars.data(), chars.size() * sizeof(uint32));
		detail::memcpy(impl->charmapGlyphs.data(), glyphs.data(), glyphs.size() * sizeof(uint32));
		impl->returnGlyph = impl->findGlyphIndex('\n');
		impl->spaceGlyph = impl->findGlyphIndex(' ');
	}

	uint32 Font::glyphsCount(const string &text) const
	{
		return utf32Length(text);
	}

	uint32 Font::glyphsCount(const char *text) const
	{
		return utf32Length(text);
	}

	uint32 Font::glyphsCount(PointerRange<const char> text) const
	{
		return utf32Length(text);
	}

	void Font::transcript(const string &text, PointerRange<uint32> glyphs) const
	{
		transcript({ text.begin(), text.end() }, glyphs);
	}

	void Font::transcript(const char *text, PointerRange<uint32> glyphs) const
	{
		uint32 s = numeric_cast<uint32>(std::strlen(text));
		transcript({ text, text + s }, glyphs);
	}

	void Font::transcript(PointerRange<const char> text, PointerRange<uint32> glyphs) const
	{
		FontImpl *impl = (FontImpl *)this;
		utf8to32(glyphs, text);
		for (uint32 &i : glyphs)
			i = impl->findGlyphIndex(i);
	}

	vec2 Font::size(PointerRange<const uint32> glyphs, const FormatStruct &format) const
	{
		vec2 mp;
		uint32 c;
		return this->size(glyphs, format, mp, c);
	}

	vec2 Font::size(PointerRange<const uint32> glyphs, const FormatStruct &format, const vec2 &mousePosition, uint32 &cursor) const
	{
		ProcessData data;
		data.mousePosition = mousePosition;
		data.format = &format;
		data.gls = glyphs.data();
		data.count = numeric_cast<uint32>(glyphs.size());
		data.outCursor = cursor;
		((FontImpl *)this)->processText(data);
		cursor = data.outCursor;
		return data.outSize;
	}

	void Font::bind(Model *model, ShaderProgram *shader)
	{
		FontImpl *impl = (FontImpl *)this;
		glActiveTexture(GL_TEXTURE0);
		impl->tex->bind();
		impl->msh = model;
		impl->msh->bind();
		impl->shr = shader;
		impl->shr->bind();
	}

	void Font::render(PointerRange<const uint32> glyphs, const FormatStruct &format, uint32 cursor)
	{
		ProcessData data;
		data.format = &format;
		data.gls = glyphs.data();
		data.count = numeric_cast<uint32>(glyphs.size());
		data.cursor = applicationTime() % 1000000 < 300000 ? m : cursor;
		data.render = true;
		((FontImpl *)this)->processText(data);
	}

	Holder<Font> newFont()
	{
		return systemArena().createImpl<Font, FontImpl>();
	}
}
