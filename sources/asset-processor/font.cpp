#include "processor.h"

#include <cage-core/hashString.h>
#include <cage-core/image.h>
#include <cage-core/imageAlgorithms.h>
#include <cage-core/rectPacking.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#define CALL(FNC, ...) { const int err = FNC(__VA_ARGS__); if (err) { CAGE_LOG_THROW(translateErrorCode(err)); CAGE_THROW_ERROR(SystemError, "FreeType " #FNC " error", err); } }
#include <msdfgen/msdfgen.h>
#include <msdfgen/ext/import-font.h>

#include <vector>
#include <algorithm>

namespace
{
	Real nominalScale;

	struct Glyph
	{
		FontHeader::GlyphData data;
		Holder<Image> png;
		uint32 pngX = 0, pngY = 0;
	};

	FT_Library library;
	FT_Face face;
	FontHeader data;
	std::vector<Glyph> glyphs;
	std::vector<Real> kerning;
	std::vector<uint32> charsetChars;
	std::vector<uint32> charsetGlyphs;
	Holder<Image> texels;

	cage::String translateErrorCode(int code)
	{
		switch (code)
#undef __FTERRORS_H__
#define FT_ERRORDEF(E,V,S) case V: return S;
#define FT_ERROR_START_LIST {
#define FT_ERROR_END_LIST };
#include FT_ERRORS_H
			CAGE_THROW_ERROR(Exception, "unknown freetype error code");
	}

	msdfgen::Vector2 from(const Vec2 &v)
	{
		return msdfgen::Vector2(v[0].value, v[1].value);
	}

	Holder<Image> glyphImage(msdfgen::Shape &shape)
	{
		if (!shape.validate())
			CAGE_THROW_ERROR(Exception, "shape validation failed");
		shape.normalize();
		shape.orientContours();
		msdfgen::edgeColoringSimple(shape, 3.0);

		const auto bounds = shape.getBounds();
		Real l = bounds.l, b = bounds.b, r = bounds.r, t = bounds.t;
		l -= .5, b -= .5;
		r += .5, t += .5;
		const Real wf = (r - l);
		const Real hf = (t - b);
		const uint32 wi = numeric_cast<uint32>(cage::ceil(wf)) + 1;
		const uint32 hi = numeric_cast<uint32>(cage::ceil(hf)) + 1;
		const Real tx = -l + .5 * (wi - wf);
		const Real ty = -b + .5 * (hi - hf);
		msdfgen::Bitmap<float, 3> msdf(wi, hi);
		msdfgen::generateMSDF(msdf, shape, msdfgen::Projection(1.0, from(Vec2(tx, ty))), 1.0);

		Holder<Image> png = newImage();
		png->initialize(wi, hi, 3);
		for (uint32 y = 0; y < png->height(); y++)
			for (uint32 x = 0; x < png->width(); x++)
				for (uint32 c = 0; c < 3; c++)
					png->value(x, y, c, msdf(x, y)[c]);
		return png;
	}

	void loadGlyphs()
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "load glyphs");
		data.glyphCount = numeric_cast<uint32>(face->num_glyphs);
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "font has " + data.glyphCount + " glyphs");
		glyphs.reserve(data.glyphCount + 10);
		glyphs.resize(data.glyphCount);
		Vec2i maxPngResolution;
		Vec2 maxGlyphSize;
		Real maxAscender, minDescender;
		msdfgen::FontHandle *handle = msdfgen::adoptFreetypeFont(face);
		for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
		{
			CALL(FT_Load_Glyph, face, glyphIndex, FT_LOAD_DEFAULT);

			// load glyph metrics
			Glyph &g = glyphs[glyphIndex];
			const FT_Glyph_Metrics &glm = face->glyph->metrics;
			g.data.size = Vec2(float(glm.width), float(glm.height)) * nominalScale;
			g.data.bearing = Vec2(float(glm.horiBearingX), float(glm.horiBearingY)) * nominalScale;
			g.data.advance = float(glm.horiAdvance) * nominalScale;

			// load glyph shape
			msdfgen::Shape shape;
			CALL(msdfgen::readFreetypeOutline, shape, &face->glyph->outline);
			if (!shape.contours.empty())
				g.png = glyphImage(shape);

			// update global data
			maxGlyphSize = max(maxGlyphSize, g.data.size);
			if (g.png)
			{
				maxPngResolution = max(maxPngResolution, g.png->resolution());
				maxAscender = max(maxAscender, g.data.bearing[1]);
				minDescender = min(minDescender, g.data.bearing[1] - g.data.size[1]);
			}
		}
		msdfgen::destroyFont(handle);
		data.firstLineOffset = maxAscender;
		data.lineHeight = maxAscender - minDescender;
		CAGE_LOG(SeverityEnum::Note, logComponentName, Stringizer() + "first line offset: " + data.firstLineOffset);
		CAGE_LOG(SeverityEnum::Note, logComponentName, Stringizer() + "line height: " + data.lineHeight);
		CAGE_LOG(SeverityEnum::Note, logComponentName, Stringizer() + "max glyph size: " + maxGlyphSize);
		CAGE_LOG(SeverityEnum::Note, logComponentName, Stringizer() + "max glyph image resolution: " + maxPngResolution);
	}

	void loadCharset()
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "load charset");
		charsetChars.reserve(1000);
		charsetGlyphs.reserve(1000);
		FT_ULong charcode;
		FT_UInt glyphIndex;
		charcode = FT_Get_First_Char(face, &glyphIndex);
		while (glyphIndex)
		{
			charsetChars.push_back(numeric_cast<uint32>(charcode));
			charsetGlyphs.push_back(numeric_cast<uint32>(glyphIndex));
			charcode = FT_Get_Next_Char(face, charcode, &glyphIndex);
		}
		data.charCount = numeric_cast<uint32>(charsetChars.size());
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "font has " + data.charCount + " characters");
	}

	void loadKerning()
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "load kerning");
		if (FT_HAS_KERNING(face))
		{
			kerning.resize(data.glyphCount * data.glyphCount);
			for (uint32 L = 0; L < data.glyphCount; L++)
			{
				for (uint32 R = 0; R < data.glyphCount; R++)
				{
					FT_Vector k;
					CALL(FT_Get_Kerning, face, L, R, FT_KERNING_UNSCALED, &k);
					kerning[L * data.glyphCount + R] = float(k.x) / face->units_per_EM;
				}
			}
			data.flags |= FontFlags::Kerning;
		}
		else
			CAGE_LOG(SeverityEnum::Info, logComponentName, "font has no kerning");
	}

	msdfgen::Shape cursorShape()
	{
		const msdfgen::Point2 points[4] = {
			{0, 0}, {0, 10}, {1, 10}, {1, 0}
		};
		msdfgen::Shape shape;
		msdfgen::Contour &c = shape.addContour();
		c.addEdge(msdfgen::EdgeHolder(points[0], points[1]));
		c.addEdge(msdfgen::EdgeHolder(points[1], points[2]));
		c.addEdge(msdfgen::EdgeHolder(points[2], points[3]));
		c.addEdge(msdfgen::EdgeHolder(points[3], points[0]));
		return shape;
	}

	void addArtificialData()
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "adding artificial data");

		bool foundReturn = false;
		for (uint32 i = 0; i < data.charCount; i++)
		{
			if (charsetChars[i] == '\n')
			{
				foundReturn = true;
				break;
			}
		}
		if (!foundReturn)
		{
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "artificially adding return character");
			uint32 idx = 0;
			while (idx < charsetChars.size() && charsetChars[idx] < '\n')
				idx++;
			charsetChars.insert(charsetChars.begin() + idx, '\n');
			charsetGlyphs.insert(charsetGlyphs.begin() + idx, uint32(m));
			data.charCount++;
		}

		{ // add cursor
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "artificially adding cursor glyph");
			data.glyphCount++;
			glyphs.resize(glyphs.size() + 1);
			Glyph &g = glyphs[glyphs.size() - 1];
			g.data.advance = 0;
			g.data.size[0] = 0.1;
			g.data.size[1] = data.lineHeight;
			g.data.bearing[0] = g.data.size[0] * -0.5;
			g.data.bearing[1] = data.firstLineOffset;
			msdfgen::Shape shape = cursorShape();
			g.png = glyphImage(shape);

			if (!kerning.empty())
			{ // compensate kerning
				std::vector<Real> k;
				k.resize(data.glyphCount * data.glyphCount);
				uint32 dgcm1 = data.glyphCount - 1;
				for (uint32 i = 0; i < dgcm1; i++)
					detail::memcpy(&k[i * data.glyphCount], &kerning[i * dgcm1], dgcm1 * sizeof(Real));
				std::swap(k, kerning);
			}
		}
	}

	void createAtlasCoordinates()
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "create atlas coordinates");
		Holder<RectPacking> packer = newRectPacking();
		uint32 area = 0;
		uint32 mgs = 0;
		uint32 count = 0;
		for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
		{
			Glyph &g = glyphs[glyphIndex];
			if (!g.png)
				continue;
			area += g.png->width() * g.png->height();
			mgs = max(mgs, max(g.png->width(), g.png->height()));
			count++;
		}
		packer->resize(count);
		count = 0;
		for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
		{
			Glyph &g = glyphs[glyphIndex];
			if (!g.png)
				continue;
			packer->data()[count++] = PackingRect{ glyphIndex, g.png->width(), g.png->height() };
		}
		uint32 res = 64;
		while (res < mgs) res += 32;
		while (res * res < area) res += 32;
		while (true)
		{
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "trying to pack into resolution " + res + "*" + res);
			RectPackingSolveConfig cfg;
			cfg.margin = 1;
			cfg.width = cfg.height = res;
			if (packer->solve(cfg))
				break;
			res += 32;
		}
		for (const auto &it : packer->data())
		{
			uint32 glyphIndex = it.id, x = it.x, y = it.y;
			CAGE_ASSERT(glyphIndex < glyphs.size());
			Glyph &g = glyphs[glyphIndex];
			CAGE_ASSERT(x < res);
			CAGE_ASSERT(y < res);
			g.pngX = x;
			g.pngY = y;
			Vec2 to = Vec2(g.pngX, g.pngY) / res;
			Vec2 ts = Vec2(g.png->width(), g.png->height()) / res;
			g.data.texUv = Vec4(to, ts);
		}
		data.texResolution = Vec2i(res);
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "texture atlas resolution " + data.texResolution[0] + "*" + data.texResolution[1]);
	}

	void createAtlasPixels()
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "create atlas pixels");
		texels = newImage();
		texels->initialize(data.texResolution[0], data.texResolution[1], 3);
		for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
		{
			Glyph &g = glyphs[glyphIndex];
			if (!g.png)
				continue;
			imageBlit(g.png.get(), texels.get(), 0, 0, g.pngX, g.pngY, g.png->width(), g.png->height());
		}
		data.texSize = numeric_cast<uint32>(texels->rawViewU8().size());
	}

	void exportData()
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "export data");

		CAGE_ASSERT(glyphs.size() == data.glyphCount);
		CAGE_ASSERT(charsetChars.size() == data.charCount);
		CAGE_ASSERT(charsetChars.size() == charsetGlyphs.size());
		CAGE_ASSERT(kerning.size() == 0 || kerning.size() == data.glyphCount * data.glyphCount);

		AssetHeader h = initializeAssetHeader();
		h.originalSize = sizeof(data) + data.texSize +
			data.glyphCount * sizeof(FontHeader::GlyphData) +
			sizeof(Real) * numeric_cast<uint32>(kerning.size()) +
			sizeof(uint32) * numeric_cast<uint32>(charsetChars.size()) +
			sizeof(uint32) * numeric_cast<uint32>(charsetGlyphs.size());

		MemoryBuffer buf;
		Serializer sr(buf);

		sr << data;
		{
			const auto &r = texels->rawViewU8();
			CAGE_ASSERT(r.size() == data.texSize);
			sr.write(bufferCast(r));
		}
		for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
			sr << glyphs[glyphIndex].data;
		if (kerning.size() > 0)
			sr.write(bufferCast<char, Real>(kerning));
		if (charsetChars.size() > 0)
		{
			sr.write(bufferCast<char, uint32>(charsetChars));
			sr.write(bufferCast<char, uint32>(charsetGlyphs));
		}

		CAGE_ASSERT(h.originalSize == buf.size());
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "buffer size (before compression): " + buf.size());
		Holder<PointerRange<char>> buf2 = compress(buf);
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "buffer size (after compression): " + buf2.size());
		h.compressedSize = buf2.size();

		Holder<File> f = writeFile(outputFileName);
		f->write(bufferView(h));
		f->write(buf2);
		f->close();
	}

	void printDebugData()
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "print debug data");

		const ConfigString fontPath("cage-asset-processor/font/path", "asset-preview");

		if (configGetBool("cage-asset-processor/font/preview"))
		{
			imageVerticalFlip(+texels);
			texels->exportFile(pathJoin(fontPath, pathReplaceInvalidCharacters(inputName) + ".png"));
		}

		if (configGetBool("cage-asset-processor/font/glyphs"))
		{ // glyphs
			FileMode fm(false, true);
			fm.textual = true;
			Holder<File> f = newFile(pathJoin(fontPath, pathReplaceInvalidCharacters(inputName) + ".glyphs.txt"), fm);
			f->writeLine(
				fill(String("glyph"), 10) +
				fill(String("tex coord"), 60) +
				fill(String("size"), 30) +
				fill(String("bearing"), 30) +
				String("advance")
			);
			for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
			{
				Glyph &g = glyphs[glyphIndex];
				f->writeLine(
					fill(String(Stringizer() + glyphIndex), 10) +
					fill(String(Stringizer() + g.data.texUv), 60) +
					fill(String(Stringizer() + g.data.size), 30) +
					fill(String(Stringizer() + g.data.bearing), 30) +
					String(Stringizer() + g.data.advance.value)
				);
			}
		}

		if (configGetBool("cage-asset-processor/font/characters"))
		{ // characters
			FileMode fm(false, true);
			fm.textual = true;
			Holder<File> f = newFile(pathJoin(fontPath, pathReplaceInvalidCharacters(inputName) + ".characters.txt"), fm);
			f->writeLine(
				fill(String("code"), 10) +
				fill(String("char"), 5) +
				String("glyph")
			);
			for (uint32 charIndex = 0; charIndex < data.charCount; charIndex++)
			{
				uint32 c = charsetChars[charIndex];
				char C = c < 256 ? c : ' ';
				f->writeLine(Stringizer() +
					fill(String(Stringizer() + c), 10) +
					fill(String({ &C, &C + 1 }), 5) +
					charsetGlyphs[charIndex]
				);
			}
		}

		if (configGetBool("cage-asset-processor/font/kerning"))
		{ // kerning
			FileMode fm(false, true);
			fm.textual = true;
			Holder<File> f = newFile(pathJoin(fontPath, pathReplaceInvalidCharacters(inputName) + ".kerning.txt"), fm);
			f->writeLine(
				fill(String("g1"), 5) +
				fill(String("g2"), 5) +
				String("kerning")
			);
			if (kerning.empty())
				f->writeLine("no data");
			else
			{
				const uint32 m = data.glyphCount;
				CAGE_ASSERT(kerning.size() == m * m);
				for (uint32 x = 0; x < m; x++)
				{
					for (uint32 y = 0; y < m; y++)
					{
						const Real k = kerning[x * m + y];
						if (k == 0)
							continue;
						f->writeLine(
							fill(String(Stringizer() + x), 5) +
							fill(String(Stringizer() + y), 5) +
							String(Stringizer() + k)
						);
					}
				}
			}
		}
	}

	void clearAll()
	{
		glyphs.clear();
		kerning.clear();
		charsetChars.clear();
		charsetGlyphs.clear();
		texels.clear();
	}

	void setSize(uint32 nominalSize)
	{
		CALL(FT_Set_Pixel_Sizes, face, nominalSize, nominalSize);
		nominalScale = 1.0 / nominalSize / 64;
	}
}

void processFont()
{
	writeLine(String("use=") + inputFile);
	if (!inputSpec.empty())
		CAGE_THROW_ERROR(Exception, "input specification must be empty");
	CALL(FT_Init_FreeType, &library);
	CALL(FT_New_Face, library, inputFileName.c_str(), 0, &face);
	if (!FT_IS_SCALABLE(face))
		CAGE_THROW_ERROR(Exception, "font is not scalable");
	CALL(FT_Select_Charmap, face, FT_ENCODING_UNICODE);
	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "units per EM: " + face->units_per_EM);
	setSize(60);
	loadGlyphs();
	loadCharset();
	loadKerning();
	addArtificialData();
	createAtlasCoordinates();
	createAtlasPixels();
	exportData();
	printDebugData();
	clearAll();
	CALL(FT_Done_FreeType, library);
}

void analyzeFont()
{
	try
	{
		CALL(FT_Init_FreeType, &library);
		CALL(FT_New_Face, library, inputFileName.c_str(), 0, &face);
		CALL(FT_Select_Charmap, face, FT_ENCODING_UNICODE);
		writeLine("cage-begin");
		writeLine("scheme=font");
		writeLine(String() + "asset=" + inputFile);
		writeLine("cage-end");
	}
	catch (...)
	{
		// do nothing
	}
	CALL(FT_Done_FreeType, library);
}
