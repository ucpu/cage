#include <algorithm>
#include <vector>

extern "C"
{
#include <SheenBidi.h>
}
#include <hb-ft.h>

#include <cage-core/assetsOnDemand.h>
#include <cage-core/concurrent.h>
#include <cage-core/hashString.h>
#include <cage-core/image.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/serialization.h>
#include <cage-core/unicode.h>
#include <cage-engine/assetStructs.h>
#include <cage-engine/font.h>
#include <cage-engine/model.h>
#include <cage-engine/opengl.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/texture.h>

#define FT_CALL(FNC, ...) \
	if (const FT_Error err = FNC(__VA_ARGS__)) \
	{ \
		CAGE_LOG_THROW(::cage::privat::translateFtErrorCode(err)); \
		CAGE_THROW_ERROR(Exception, "FreeType " #FNC " error"); \
	}

namespace
{
	cage::String translateFtErrorCodeImpl(FT_Error code)
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
	namespace privat
	{
		CAGE_CORE_API bool unicodeIsWhitespace(uint32 c);

		CAGE_ENGINE_API cage::String translateFtErrorCode(FT_Error code)
		{
			return translateFtErrorCodeImpl(code);
		}
	}

	namespace
	{
		FT_Library ftLibrary;
		Holder<Mutex> ftMutex = newMutex();
		class FtInitializer
		{
		public:
			FtInitializer()
			{
				FT_CALL(FT_Init_FreeType, &ftLibrary);
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
			std::pair<PointerRange<const hb_glyph_info_t>, PointerRange<const hb_glyph_position_t>> getRanges() const
			{
				uint32 cnt = 0;
				static_assert(sizeof(cnt) == sizeof(unsigned int));
				const hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(buffer, &cnt);
				const hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(buffer, &cnt);
				return { { infos, infos + cnt }, { positions, positions + cnt } };
			}

		private:
			hb_buffer_t *buffer = nullptr;
		};

		constexpr uint32 MaxCharacters = 128;
		struct Instance
		{
			Vec4 wrld;
			Vec4 texUv;
		};

		class FontImpl : public Font
		{
		public:
			FontHeader header = {};
			MemoryBuffer ftFile;
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
				uint32 i = glyphId > 4 ? glyphId - 4 : 0;
				if (i >= glyphs.size())
					i = glyphs.size() - 1;
				CAGE_ASSERT(i < glyphs.size());
				if (glyphId < glyphs[i].glyphId)
				{
					while (true)
					{
						const uint32 g = glyphs[i].glyphId;
						if (g == glyphId)
							return i;
						if (g < glyphId)
							return m;
						if (i == 0)
							return m;
						i--;
					}
				}
				else
				{
					while (true)
					{
						const uint32 g = glyphs[i].glyphId;
						if (g == glyphId)
							return i;
						if (g > glyphId)
							return m;
						if (i + 1 >= glyphs.size())
							return m;
						i++;
					}
				}
			}

			static void shortenLine(PointerRange<const uint32> text, SBRun &run)
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

			static bool allowLineBreak(uint32 a, uint32 b) { return privat::unicodeIsWhitespace(a) || b == '\n'; }

			// a, b is inclusive range of glyphs in the line
			// returns utf32 index in input text (the cursor goes before the character: 0 is before the first character of the text)
			static uint32 findCursorInLine(uint32 a, uint32 b, Real p, PointerRange<const FontLayoutGlyph> glyphs)
			{
				CAGE_ASSERT(a < glyphs.size());
				CAGE_ASSERT(b < glyphs.size());
				if (p < glyphs[a].wrld[0])
				{
					const uint32 c = glyphs[a].cluster;
					return c == 0 ? 0 : c - 1;
				}
				if (p > glyphs[b].wrld[0])
					return glyphs[b].cluster + 1;
				for (uint32 i = a; i < b; i++)
				{
					if (p < glyphs[i].wrld[0] + glyphs[i].wrld[2])
						return glyphs[i].cluster;
				}
				return glyphs[b].cluster + 1;
			}

			FontLayoutResult layout(PointerRange<const char> text8, const FontFormat &format, Vec2 cursorPoint, uint32 cursorIndex) const
			{
				const auto text32 = utf8to32(text8);
				return layout(text32, format, cursorPoint, cursorIndex);
			}

			FontLayoutResult layout(PointerRange<const uint32> text32, const FontFormat &format, Vec2 cursorPoint, uint32 cursorIndex) const
			{
				CAGE_ASSERT(!valid(cursorPoint) || (cursorIndex == m));
				PointerRangeHolder<FontLayoutGlyph> glyphs;
				glyphs.reserve(text32.size() + 10);
				Vec2 pos = Vec2(0, header.lineOffset * format.size);

				const auto &addCursor = [&]()
				{
					FontLayoutGlyph g;
					g.index = findArrayIndex(uint32(-2)); // cursor glyph
					CAGE_ASSERT(g.index != m);
					g.cluster = cursorIndex;
					g.wrld = Vec4(pos + Vec2(-0.07 * format.size, 0), Vec2(0.14, header.lineHeight) * format.size);
					glyphs.push_back(g);
				};

				const auto &emptyResult = [&]()
				{
					CAGE_ASSERT(glyphs.empty());
					FontLayoutResult res;
					if (cursorIndex == 0)
					{
						addCursor();
						res.glyphs = std::move(glyphs);
					}
					res.size = Vec2(0, header.lineHeight * max(format.lineSpacing, 1) * format.size);
					return res;
				};
				if (text32.empty())
					return emptyResult();

				HarfBuffer hb;
				const BidiAlgorithm bidiAlgorithm(SBCodepointSequence{ SBStringEncodingUTF32, (void *)text32.data(), text32.size() });
				SBUInteger paragraphStart = 0;
				std::vector<uint32> glyphsCountsInLines; // glyphs counts at the end of each line
				glyphsCountsInLines.reserve(5);
				const Real scale = header.nominalScale * format.size;
				const Real lineAdvance = -header.lineHeight * format.lineSpacing * format.size;
				Vec2 prevPos = pos;

				// do the shaping
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

						uint32 wrapIndex = m;
						Real wrapPos = 0;
						for (SBRun run : line.getRuns())
						{
							shortenLine(text32, run);
							if (run.length == 0)
								continue;

							hb_buffer_clear_contents(hb());
							hb_buffer_add_utf32(hb(), text32.data(), text32.size(), run.offset, run.length);
							hb_buffer_set_direction(hb(), (run.level % 2) ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
							hb_buffer_guess_segment_properties(hb());
							hb_shape(font, hb(), nullptr, 0);

							const auto [infos, positions] = hb.getRanges();
							hb_glyph_extents_t extents;
							for (uint32 i = 0; i < infos.size(); i++)
							{
								// cursor
								{
									const bool checkBefore = i == 0 ? (infos[i].cluster == cursorIndex) : (infos[i - 1].cluster < cursorIndex);
									const bool checkAfter = infos[i].cluster >= cursorIndex;
									if (checkBefore && checkAfter)
										addCursor();
								}

								// place glyph
								const auto gp = positions[i];
								const uint32 ai = findArrayIndex(infos[i].codepoint);
								if (ai != m && hb_font_get_glyph_extents(font, infos[i].codepoint, &extents))
								{
									const Vec2 p = Vec2(gp.x_offset + extents.x_bearing, gp.y_offset + extents.y_bearing + extents.height) * scale;
									const Vec2 s = Vec2(extents.width, -extents.height) * scale;
									glyphs.push_back({ Vec4(pos + p, s), ai, infos[i].cluster });
								}

								// advance
								pos += Vec2(gp.x_advance, gp.y_advance) * scale;

								// detect possible word wrap
								if (!glyphs.empty())
								{
									const bool differentCluster = i + 1 >= infos.size() || infos[i].cluster != infos[i + 1].cluster;
									const bool breakingCharacter = allowLineBreak(text32[infos[i].cluster], infos[i].cluster + 1 < text32.size() ? text32[infos[i].cluster + 1] : '\n');
									if (differentCluster && breakingCharacter)
									{
										if (wrapIndex != m && glyphs.back().wrld[0] + glyphs.back().wrld[2] > format.wrapWidth)
										{
											const Vec4 off = Vec4(-wrapPos, lineAdvance, 0, 0);
											for (uint32 i = wrapIndex; i < glyphs.size(); i++)
												glyphs[i].wrld += off;
											pos += Vec2(off);
											glyphsCountsInLines.push_back(wrapIndex);
										}
										wrapIndex = glyphs.size();
										wrapPos = pos[0];
									}
								}
							}
						}

						// next line
						prevPos = pos;
						pos[0] = 0;
						pos[1] += lineAdvance;
						glyphsCountsInLines.push_back(glyphs.size());
					}
				}

				// cursor at the end
				if (cursorIndex == text32.size())
				{
					pos = prevPos;
					addCursor();
				}

				if (glyphs.empty())
					return emptyResult();

				// overall width
				Real width;
				for (const auto &it : glyphs)
					width = max(width, it.wrld[0] + it.wrld[2]);
				if (format.wrapWidth != Real::Infinity())
					width = max(width, format.wrapWidth);

				// alignment
				if (format.align != TextAlignEnum::Left)
				{
					uint32 lineStart = 0;
					for (uint32 lineEnd : glyphsCountsInLines)
					{
						if (lineEnd > lineStart)
						{
							const Real a = glyphs[lineStart].wrld[0];
							const Real b = glyphs[lineEnd - 1].wrld[0] + glyphs[lineEnd - 1].wrld[2];
							Real off = 0;
							switch (format.align)
							{
								case TextAlignEnum::Right:
									off = width - b;
									break;
								case TextAlignEnum::Center:
									off = (width - b + a) * 0.5 - a;
									break;
							}
							for (uint32 i = lineStart; i < lineEnd; i++)
								glyphs[i].wrld[0] += off;
						}
						lineStart = lineEnd;
					}
				}

				FontLayoutResult res;
				res.size = Vec2(width, header.lineHeight * max(glyphsCountsInLines.size() * format.lineSpacing, 1) * format.size);

				// cursor position
				if (valid(cursorPoint))
				{
					if (cursorPoint[0] >= 0 && cursorPoint[1] >= 0 && cursorPoint[0] < res.size[0] && cursorPoint[1] < res.size[1])
					{
						const uint32 l = numeric_cast<uint32>(cursorPoint[1] / -lineAdvance);
						CAGE_ASSERT(l < glyphsCountsInLines.size());
						const uint32 a = l > 0 ? glyphsCountsInLines[l - 1] : 0;
						const uint32 b = glyphsCountsInLines[l] - 1;
						res.cursor = findCursorInLine(a, b, cursorPoint[0], glyphs);
					}
					else
						res.cursor = m;
				}
				else if (cursorIndex <= text32.size())
					res.cursor = cursorIndex;

				// sort glyphs by image
				std::sort(glyphs.begin(), glyphs.end(), [this](const FontLayoutGlyph &a, const FontLayoutGlyph &b) { return this->glyphs[a.index].image < this->glyphs[b.index].image; });

				res.glyphs = std::move(glyphs);
				return res;
			}

			void render(RenderQueue *queue, AssetsOnDemand *assets, const FontLayoutResult &layout) const
			{
				if (layout.glyphs.empty())
					return;
				Holder<Model> model = assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
				if (!model)
					return;
				Instance insts[MaxCharacters];
				uint32 image = glyphs[layout.glyphs[0].index].image;
				uint32 i = 0;
				const auto &dispatch = [&]()
				{
					if (i == 0)
						return;
					Holder<Texture> t = assets->get<AssetSchemeIndexTexture, Texture>(image);
					if (!t)
						return;
					queue->bind(t, 0);
					PointerRange<Instance> r(insts, insts + i);
					queue->universalUniformArray<Instance>(r, 2);
					queue->draw(model, i);
				};

				for (const auto &it : layout.glyphs)
				{
					const auto &g = glyphs[it.index];
					if (g.image != image || i == MaxCharacters)
					{
						dispatch();
						image = g.image;
						i = 0;
					}
					insts[i].wrld = it.wrld;
					insts[i].texUv = g.texUv;
					i++;
				}
				dispatch();
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
		CAGE_ASSERT(impl->glyphs.empty());
		CAGE_ASSERT(!impl->face);
		CAGE_ASSERT(!impl->font);

		Deserializer des(buffer);
		des >> impl->header;

		impl->ftFile.resize(impl->header.ftSize);
		des.read(impl->ftFile);

		impl->glyphs.resize(impl->header.glyphsCount);
		for (auto &it : impl->glyphs)
			des >> it;

		CAGE_ASSERT(des.available() == 0);

		{
			ScopeLock _(ftMutex);
			FT_CALL(FT_New_Memory_Face, ftLibrary, (FT_Byte *)impl->ftFile.data(), impl->ftFile.size(), 0, &impl->face);
		}
		FT_CALL(FT_Select_Charmap, impl->face, FT_ENCODING_UNICODE);
		FT_CALL(FT_Set_Pixel_Sizes, impl->face, impl->header.nominalSize, impl->header.nominalSize);
		impl->font = hb_ft_font_create(impl->face, nullptr);

		CAGE_ASSERT(impl->findArrayIndex(uint32(-2)) != m); // verify that the font contains a glyph for the cursor
	}

	FontLayoutResult Font::layout(PointerRange<const char> text, const FontFormat &format, Vec2 cursorPoint) const
	{
		const FontImpl *impl = (const FontImpl *)this;
		return impl->layout(text, format, cursorPoint, m);
	}

	FontLayoutResult Font::layout(PointerRange<const char> text, const FontFormat &format, uint32 cursorIndex) const
	{
		const FontImpl *impl = (const FontImpl *)this;
		return impl->layout(text, format, Vec2::Nan(), cursorIndex);
	}

	FontLayoutResult Font::layout(PointerRange<const uint32> text, const FontFormat &format, Vec2 cursorPoint) const
	{
		const FontImpl *impl = (const FontImpl *)this;
		return impl->layout(text, format, cursorPoint, m);
	}

	FontLayoutResult Font::layout(PointerRange<const uint32> text, const FontFormat &format, uint32 cursorIndex) const
	{
		const FontImpl *impl = (const FontImpl *)this;
		return impl->layout(text, format, Vec2::Nan(), cursorIndex);
	}

	void Font::render(RenderQueue *queue, AssetsOnDemand *assets, const FontLayoutResult &layout) const
	{
		const FontImpl *impl = (const FontImpl *)this;
		impl->render(queue, assets, layout);
	}

	Holder<Font> newFont()
	{
		return systemMemory().createImpl<Font, FontImpl>();
	}
}
