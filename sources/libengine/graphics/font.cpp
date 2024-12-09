#include <algorithm>
#include <vector>

extern "C"
{
#include <SheenBidi.h>
}
#include <hb-ft.h>

#include <cage-core/concurrent.h>
#include <cage-core/image.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/serialization.h>
#include <cage-core/unicode.h>
#include <cage-engine/assetStructs.h>
#include <cage-engine/font.h>
#include <cage-engine/opengl.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/texture.h>

namespace
{
	cage::String translateErrorCode(int code)
	{
#undef __FTERRORS_H__
		switch (code)
#define FT_ERROR_START_LIST {
#define FT_ERRORDEF(E, V, S) \
	case V: \
		return S;
#define FT_ERROR_END_LIST \
	} \
	;
#include FT_ERRORS_H
			CAGE_THROW_ERROR(cage::Exception, "unknown freetype error code");
	}
}

namespace cage
{
	namespace
	{
		FT_Library ftLibrary;
		Holder<Mutex> ftMutex = newMutex();
		class FtInitializer
		{
		public:
			FtInitializer()
			{
				if (FT_Init_FreeType(&ftLibrary))
					CAGE_THROW_ERROR(Exception, "failed to initialize FreeType");
				hb_language_get_default(); // initialize the locale before threads
			}
			~FtInitializer()
			{
				ScopeLock _(ftMutex);
				FT_Done_FreeType(ftLibrary);
			}
		} ftInitializer;

		struct BidiAlgorithm : private Noncopyable
		{
			explicit BidiAlgorithm(SBCodepointSequence codepoints) { algorithm = SBAlgorithmCreate(&codepoints); }
			~BidiAlgorithm() { SBAlgorithmRelease(algorithm); }
			SBAlgorithmRef operator()() const { return algorithm; };

		private:
			SBAlgorithmRef algorithm = nullptr;
		};

		struct BidiParagraph : private Noncopyable
		{
			explicit BidiParagraph(SBAlgorithmRef bidiAlgorithm, SBUInteger paragraphStart) { paragraph = SBAlgorithmCreateParagraph(bidiAlgorithm, paragraphStart, INT32_MAX, SBLevelDefaultLTR); }
			~BidiParagraph() { SBParagraphRelease(paragraph); }
			SBParagraphRef operator()() const { return paragraph; };

		private:
			SBParagraphRef paragraph = nullptr;
		};

		struct BidiLine : private Noncopyable
		{
			explicit BidiLine(SBParagraphRef paragraph, SBUInteger lineStart, SBUInteger paragraphLength) { line = SBParagraphCreateLine(paragraph, lineStart, paragraphLength); }
			~BidiLine() { SBLineRelease(line); }
			SBLineRef operator()() const { return line; };
			PointerRange<const SBRun> getRuns() const
			{
				SBUInteger runsCount = SBLineGetRunCount(line);
				const SBRun *runArray = SBLineGetRunsPtr(line);
				return { runArray, runArray + runsCount };
			}

		private:
			SBLineRef line = nullptr;
		};

		struct HarfBuffer : private Noncopyable
		{
			explicit HarfBuffer() { buffer = hb_buffer_create(); }
			~HarfBuffer() { hb_buffer_destroy(buffer); }
			hb_buffer_t *operator()() const { return buffer; };

		private:
			hb_buffer_t *buffer = nullptr;
		};

		constexpr uint32 MaxCharacters = 128;
		struct Instance
		{
			Vec4 wrld;
			Vec4 texUv;
		};

		struct TextureImage
		{
			Holder<Image> img;
			Holder<Texture> tex;
		};

		class FontImpl : public Font
		{
		public:
			FontHeader header = {};
			MemoryBuffer ftFile;
			std::vector<TextureImage> images;
			std::vector<FontHeader::GlyphData> glyphs;
			FT_Face face = nullptr;
			hb_font_t *font = nullptr;

			~FontImpl()
			{
				if (font)
				{
					hb_font_destroy(font);
					font = nullptr;
				}
				if (face)
				{
					ScopeLock _(ftMutex);
					FT_Done_Face(face);
					face = nullptr;
				}
			}

			uint32 findArrayIndex(uint32 glyphId) const
			{
				FontHeader::GlyphData g;
				g.glyphId = glyphId;
				auto it = std::lower_bound(glyphs.begin(), glyphs.end(), g, [](const FontHeader::GlyphData &a, const FontHeader::GlyphData &b) { return a.glyphId < b.glyphId; });
				if (it == glyphs.end())
					return m;
				if (it->glyphId != glyphId)
					return m;
				return it - glyphs.begin();
			}
		};
	}

	void Font::setDebugName(const String &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
	}

	void Font::importBuffer(PointerRange<const char> buffer)
	{
		FontImpl *impl = (FontImpl *)this;
		CAGE_ASSERT(impl->images.empty());
		CAGE_ASSERT(impl->glyphs.empty());

		Deserializer des(buffer);
		des >> impl->header;

		impl->ftFile.resize(impl->header.ftSize);
		des.read(impl->ftFile);

		MemoryBuffer tmp;
		impl->images.reserve(impl->header.imagesCount);
		for (uint32 i = 0; i < impl->header.imagesCount; i++)
		{
			uint32 s = 0;
			des >> s;
			tmp.resize(s);
			des.read(tmp);
			Holder<Image> img = newImage();
			img->importBuffer(tmp);
			img->colorConfig.gammaSpace = GammaSpaceEnum::Linear;
			Holder<Texture> tex = newTexture();
			tex->importImage(+img);
			tex->filters(GL_LINEAR, GL_LINEAR, 16);
			impl->images.push_back({ std::move(img), std::move(tex) });
		}

		impl->glyphs.resize(impl->header.glyphsCount);
		for (auto &it : impl->glyphs)
			des >> it;

		{
			ScopeLock _(ftMutex);
			if (auto err = FT_New_Memory_Face(ftLibrary, (FT_Byte *)impl->ftFile.data(), impl->ftFile.size(), 0, &impl->face))
			{
				CAGE_LOG_THROW(translateErrorCode(err));
				CAGE_THROW_ERROR(Exception, "failed loading font with FreeType");
			}
		}
		if (auto err = FT_Select_Charmap(impl->face, FT_ENCODING_UNICODE))
		{
			CAGE_LOG_THROW(translateErrorCode(err));
			CAGE_THROW_ERROR(Exception, "failed to select charmap in FreeType");
		}
		if (auto err = FT_Set_Pixel_Sizes(impl->face, impl->header.nominalSize, impl->header.nominalSize))
		{
			CAGE_LOG_THROW(translateErrorCode(err));
			CAGE_THROW_ERROR(Exception, "failed to set font size in FreeType");
		}
		impl->font = hb_ft_font_create(impl->face, nullptr);
	}

	void shortenLine(PointerRange<const char> text, SBRun &run)
	{
		CAGE_ASSERT(run.offset + run.length <= text.size());
		while (run.length > 0)
		{
			switch (text[run.offset + run.length - 1])
			{
				case '\n':
				case '\r':
					run.length--;
					break;
				default:
					return;
			}
		}
	}

	FontLayoutResult Font::layout(PointerRange<const char> text, const FontFormat &format) const
	{
		if (text.empty())
			return {}; // todo cursor

		const FontImpl *impl = (const FontImpl *)this;

		HarfBuffer hb;
		PointerRangeHolder<FontLayoutGlyph> glyphs;
		glyphs.reserve(text.size() + 10);
		const Real scale = impl->header.nominalScale * format.size;

		const BidiAlgorithm bidiAlgorithm(SBCodepointSequence{ SBStringEncodingUTF8, (void *)text.data(), text.size() });
		SBUInteger paragraphStart = 0;
		uint32 lineIndex = 0;
		while (true)
		{
			const BidiParagraph paragraph(bidiAlgorithm(), paragraphStart);
			if (!paragraph())
				break;

			SBUInteger paragraphLength = SBParagraphGetLength(paragraph());
			SBUInteger lineStart = paragraphStart;
			paragraphStart += paragraphLength;

			while (true)
			{
				const BidiLine line(paragraph(), lineStart, paragraphLength);
				if (!line())
					break;

				SBUInteger lineLength = SBLineGetLength(line());
				lineStart += lineLength;
				paragraphLength -= lineLength;

				Vec2 pos = Vec2(0, (impl->header.lineOffset - impl->header.lineHeight * lineIndex * format.lineSpacing) * format.size);
				for (SBRun run : line.getRuns())
				{
					shortenLine(text, run);
					if (run.length == 0)
						continue;

					hb_buffer_clear_contents(hb());
					hb_buffer_add_utf8(hb(), text.data(), text.size(), run.offset, run.length);
					hb_buffer_set_direction(hb(), (run.level % 2) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
					hb_buffer_guess_segment_properties(hb());
					hb_shape(impl->font, hb(), nullptr, 0);

					uint32 cnt = 0;
					static_assert(sizeof(cnt) == sizeof(unsigned int));
					const hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(hb(), &cnt);
					const hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(hb(), &cnt);
					hb_glyph_extents_t extents;
					for (uint32 i = 0; i < cnt; i++)
					{
						const auto gp = positions[i];
						const uint32 ai = impl->findArrayIndex(infos[i].codepoint);
						if (ai != m && hb_font_get_glyph_extents(impl->font, infos[i].codepoint, &extents))
						{
							const Vec2 p = Vec2(gp.x_offset + extents.x_bearing, gp.y_offset + extents.y_bearing + extents.height) * scale;
							const Vec2 s = Vec2(extents.width, -extents.height) * scale;
							glyphs.push_back({ Vec4(pos + p, s), ai });
						}
						pos += Vec2(gp.x_advance, gp.y_advance) * scale;
					}
				}
				lineIndex++;
			}
		}

		Vec2 a, b;
		for (const auto &it : glyphs)
		{
			const Vec2 l = Vec2(it.wrld);
			const Vec2 r = Vec2(it.wrld) + Vec2(it.wrld[2], it.wrld[3]);
			a = min(a, min(l, r));
			b = max(b, max(l, r));
		}

		FontLayoutResult res;
		res.glyphs = std::move(glyphs);
		res.size = Vec2(b[0], impl->header.lineHeight * max(lineIndex * format.lineSpacing, 1) * format.size);
		return res;
	}

	void Font::render(RenderQueue *queue, const Holder<Model> &model, const FontLayoutResult &layout) const
	{
		if (layout.glyphs.empty())
			return;

		const FontImpl *impl = (const FontImpl *)this;

		Instance insts[MaxCharacters];
		uint32 image = impl->glyphs[layout.glyphs[0].index].imageIndex;
		uint32 i = 0;
		const auto &dispatch = [&]()
		{
			queue->bind(impl->images[image].tex, 0);
			if (i > 0)
			{
				PointerRange<Instance> r(insts, insts + i);
				queue->universalUniformArray<Instance>(r, 2);
				queue->draw(model, i);
			}
		};

		for (const auto &it : layout.glyphs)
		{
			const auto &g = impl->glyphs[it.index];
			if (g.imageIndex != image || i == MaxCharacters)
			{
				dispatch();
				image = g.imageIndex;
				i = 0;
			}
			insts[i].wrld = it.wrld;
			insts[i].texUv = g.texUv;
			i++;
		}
		dispatch();
	}

	Holder<Font> newFont()
	{
		return systemMemory().createImpl<Font, FontImpl>();
	}
}
