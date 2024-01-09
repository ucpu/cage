#include <vector>
#include <cstring>

#include <cage-core/utf.h>
#include <cage-engine/assetStructs.h>
#include <cage-engine/font.h>
#include <cage-engine/opengl.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/texture.h>

namespace cage
{
	namespace
	{
		constexpr uint32 MaxCharacters = 512;

		struct Instance
		{
			Vec4 wrld;
			Vec4 text;

			Instance(Real x, Real y, const FontHeader::GlyphData &g)
			{
				text = g.texUv;
				wrld[0] = x + g.bearing[0];
				wrld[1] = y + g.bearing[1] - g.size[1];
				wrld[2] = g.size[0];
				wrld[3] = g.size[1];
			}
		};

		struct ProcessData
		{
			Holder<Model> model;
			RenderQueue *renderQueue = nullptr;
			std::vector<Instance> instances;
			PointerRange<const uint32> glyphs;
			Vec2 mousePosition = Vec2::Nan();
			Vec2 outSize;
			const FontFormat *format = nullptr;
			uint32 outCursor = 0;
			uint32 cursor = m;
		};

		class FontImpl : public Font
		{
		public:
			std::vector<FontHeader::GlyphData> glyphsArray;
			std::vector<Real> kerning;
			std::vector<uint32> charmapChars;
			std::vector<uint32> charmapGlyphs;

			Holder<Texture> tex;

			uint32 spaceGlyph = 0;
			uint32 returnGlyph = 0;
			uint32 cursorGlyph = m;
			Real lineHeight = 0;
			Real firstLineOffset = 0;

			FontImpl() { tex = newTexture(); }

			FontHeader::GlyphData getGlyph(uint32 glyphIndex, Real size) const
			{
				FontHeader::GlyphData r = glyphsArray[glyphIndex];
				r.advance *= size;
				r.bearing *= size;
				r.size *= size;
				return r;
			}

			Real findKerning(uint32 L, uint32 R, Real size) const
			{
				CAGE_ASSERT(L < glyphsArray.size() && R < glyphsArray.size());
				if (kerning.empty() || glyphsArray.empty())
					return 0;
				const uint32 s = numeric_cast<uint32>(glyphsArray.size());
				return kerning[L * s + R] * size;
			}

			uint32 findGlyphIndex(uint32 character) const
			{
				CAGE_ASSERT(charmapChars.size());
				auto it = std::lower_bound(charmapChars.begin(), charmapChars.end(), character);
				if (*it != character)
					return 0;
				return charmapGlyphs[it - charmapChars.begin()];
			}

			void processCursor(ProcessData &data, const uint32 *begin, Real x, Real lineY) const
			{
				if (data.renderQueue && begin == data.glyphs.data() + data.cursor)
				{
					FontHeader::GlyphData g = getGlyph(cursorGlyph, data.format->size);
					data.instances.emplace_back(x, lineY, g);
				}
			}

			void processLine(ProcessData &data, const uint32 *begin, const uint32 *end, Real lineWidth, Real lineY, Real lineYCursor, Real actualLineHeight) const
			{
				Real x;
				switch (data.format->align)
				{
					case TextAlignEnum::Left:
						break;
					case TextAlignEnum::Right:
						x = data.format->wrapWidth - lineWidth;
						break;
					case TextAlignEnum::Center:
						x = (data.format->wrapWidth - lineWidth) / 2;
						break;
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid align enum value");
				}

				const Vec2 mousePos = data.mousePosition + Vec2(-x, lineYCursor);
				const bool mouseInLine = mousePos[1] >= 0 && mousePos[1] <= actualLineHeight;
				if (!data.renderQueue && !mouseInLine)
					return;
				if (mouseInLine)
				{
					if (mousePos[0] < 0)
						data.outCursor = numeric_cast<uint32>(begin - data.glyphs.data());
					else if (mousePos[0] >= lineWidth)
						data.outCursor = numeric_cast<uint32>(end - data.glyphs.data());
				}

				uint32 prev = 0;
				while (begin != end)
				{
					processCursor(data, begin, x, lineY);
					const FontHeader::GlyphData g = getGlyph(*begin, data.format->size);
					const Real k = findKerning(prev, *begin, data.format->size);
					prev = *begin++;
					if (data.renderQueue)
						data.instances.emplace_back(x + k, lineY, g);
					if (mouseInLine && data.mousePosition[0] >= x && data.mousePosition[0] < x + k + g.advance)
						data.outCursor = numeric_cast<uint32>(begin - data.glyphs.data());
					x += k + g.advance;
				}
				processCursor(data, begin, x, lineY);
			}

			void processText(ProcessData &data) const
			{
				CAGE_ASSERT(data.format->align <= TextAlignEnum::Center);
				CAGE_ASSERT(data.format->wrapWidth > 0);
				CAGE_ASSERT(data.format->size > 0);
				CAGE_ASSERT(data.format->lineSpacing >= 0);
				data.instances.reserve(data.glyphs.size() + 1);
				const uint32 *const totalEnd = data.glyphs.end();
				const uint32 *it = data.glyphs.begin();
				const Real actualLineHeight = lineHeight * data.format->lineSpacing * data.format->size;
				Real lineY = (firstLineOffset - lineHeight * (data.format->lineSpacing - 1) * 0.5) * data.format->size;
				Real lineYCursor;

				if (data.glyphs.empty())
				{ // process cursor
					processLine(data, it, totalEnd, 0, lineY, lineYCursor, actualLineHeight);
				}

				while (it != totalEnd)
				{
					const uint32 *const lineStart = it;
					const uint32 *lineEnd = it;
					Real lineWidth = 0;
					Real itWidth = 0;

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
						const Real w = getGlyph(*it, data.format->size).advance + findKerning(it == lineStart ? 0 : it[-1], *it, data.format->size);
						if (it != lineStart && itWidth + w > data.format->wrapWidth + (!!data.renderQueue * data.format->size * 1e-5))
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

					processLine(data, lineStart, lineEnd, lineWidth, lineY, lineYCursor, actualLineHeight);
					data.outSize[0] = max(data.outSize[0], lineWidth);
					data.outSize[1] += actualLineHeight;
					lineY -= actualLineHeight;
					lineYCursor -= actualLineHeight;
				}

				if (data.renderQueue)
				{
					data.renderQueue->bind(tex, 0);
					const uint32 s = numeric_cast<uint32>(data.instances.size());
					const uint32 a = s / MaxCharacters;
					const uint32 b = s - a * MaxCharacters;
					for (uint32 i = 0; i < a; i++)
					{
						const auto p = data.instances.data() + i * MaxCharacters;
						PointerRange<Instance> r = { p, p + MaxCharacters };
						data.renderQueue->universalUniformArray<Instance>(r, 2);
						data.renderQueue->draw(data.model, MaxCharacters);
					}
					if (b)
					{
						const auto p = data.instances.data() + a * MaxCharacters;
						PointerRange<Instance> r = { p, p + b };
						data.renderQueue->universalUniformArray<Instance>(r, 2);
						data.renderQueue->draw(data.model, b);
					}
				}
			}
		};
	}

	void Font::setDebugName(const String &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
		FontImpl *impl = (FontImpl *)this;
		impl->tex->setDebugName(name);
	}

	void Font::setLine(Real lineHeight, Real firstLineOffset)
	{
		FontImpl *impl = (FontImpl *)this;
		impl->lineHeight = lineHeight;
		impl->firstLineOffset = -firstLineOffset;
	}

	void Font::setImage(Vec2i resolution, PointerRange<const char> buffer)
	{
		FontImpl *impl = (FontImpl *)this;
		impl->tex->filters(GL_LINEAR, GL_LINEAR, 0);
		impl->tex->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		const uint32 bpp = numeric_cast<uint32>(buffer.size() / (resolution[0] * resolution[1]));
		CAGE_ASSERT(resolution[0] * resolution[1] * bpp == buffer.size());
		switch (bpp)
		{
			case 1:
				impl->tex->initialize(resolution, 1, GL_R8);
				impl->tex->image2d(0, GL_RED, GL_UNSIGNED_BYTE, buffer);
				break;
			case 2:
				impl->tex->initialize(resolution, 1, GL_R16);
				impl->tex->image2d(0, GL_RED, GL_UNSIGNED_SHORT, buffer);
				break;
			case 3:
				impl->tex->initialize(resolution, 1, GL_RGB8);
				impl->tex->image2d(0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
				break;
			case 4:
				impl->tex->initialize(resolution, 1, GL_R32F);
				impl->tex->image2d(0, GL_RED, GL_FLOAT, buffer);
				break;
			case 6:
				impl->tex->initialize(resolution, 1, GL_RGB16);
				impl->tex->image2d(0, GL_RGB, GL_UNSIGNED_SHORT, buffer);
				break;
			case 12:
				impl->tex->initialize(resolution, 1, GL_RGB32F);
				impl->tex->image2d(0, GL_RGB, GL_FLOAT, buffer);
				break;
			default:
				CAGE_THROW_ERROR(Exception, "font: unsupported image bpp");
		}
	}

	void Font::setGlyphs(PointerRange<const char> buffer, PointerRange<const Real> kerning)
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
			detail::memcpy(impl->kerning.data(), kerning.data(), kerning.size() * sizeof(Real));
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

	uint32 Font::glyphsCount(const String &text) const
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

	void Font::transcript(const String &text, PointerRange<uint32> glyphs) const
	{
		transcript({ text.begin(), text.end() }, glyphs);
	}

	void Font::transcript(const char *text, PointerRange<uint32> glyphs) const
	{
		const uint32 s = numeric_cast<uint32>(std::strlen(text));
		transcript({ text, text + s }, glyphs);
	}

	void Font::transcript(PointerRange<const char> text, PointerRange<uint32> glyphs) const
	{
		const FontImpl *impl = (const FontImpl *)this;
		utf8to32(glyphs, text);
		for (uint32 &i : glyphs)
			i = impl->findGlyphIndex(i);
	}

	Holder<PointerRange<uint32>> Font::transcript(const String &text) const
	{
		return transcript({ text.begin(), text.end() });
	}

	Holder<PointerRange<uint32>> Font::transcript(const char *text) const
	{
		const uint32 s = numeric_cast<uint32>(std::strlen(text));
		return transcript({ text, text + s });
	}

	Holder<PointerRange<uint32>> Font::transcript(PointerRange<const char> text) const
	{
		const FontImpl *impl = (const FontImpl *)this;
		auto glyphs = utf8to32(text);
		for (uint32 &i : glyphs)
			i = impl->findGlyphIndex(i);
		return glyphs;
	}

	Vec2 Font::size(PointerRange<const uint32> glyphs, const FontFormat &format) const
	{
		Vec2 mp;
		uint32 c;
		return this->size(glyphs, format, mp, c);
	}

	Vec2 Font::size(PointerRange<const uint32> glyphs, const FontFormat &format, const Vec2 &mousePosition, uint32 &cursor) const
	{
		const FontImpl *impl = (const FontImpl *)this;
		ProcessData data;
		data.mousePosition = mousePosition;
		data.format = &format;
		data.glyphs = glyphs;
		data.outCursor = cursor;
		impl->processText(data);
		cursor = data.outCursor;
		return data.outSize;
	}

	void Font::render(RenderQueue *queue, const Holder<Model> &model, PointerRange<const uint32> glyphs, const FontFormat &format, uint32 cursor) const
	{
		if (format.wrapWidth <= 0 || format.size <= 0)
			return;
		const FontImpl *impl = (const FontImpl *)this;
		ProcessData data;
		data.model = model.share();
		data.renderQueue = queue;
		data.format = &format;
		data.glyphs = glyphs;
		data.cursor = applicationTime() % 1000000 < 300000 ? m : cursor;
		impl->processText(data);
	}

	Holder<Font> newFont()
	{
		return systemMemory().createImpl<Font, FontImpl>();
	}
}
