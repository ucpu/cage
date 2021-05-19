#include <cage-core/hashString.h>
#include <cage-core/image.h>
#include <cage-core/rectPacking.h>

#include "processor.h"

#include <msdfgen.h>
#include <msdfgen-ext.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#include <vector>

#define CALL(FNC, ...) { int err = FNC(__VA_ARGS__); if (err) { CAGE_LOG_THROW(translateErrorCode(err)); CAGE_THROW_ERROR(SystemError, "FreeType " #FNC " error", err); } }

namespace
{
	const cage::string translateErrorCode(int code)
	{
		switch (code)
#undef __FTERRORS_H__
#define FT_ERRORDEF(E,V,S) case V: return S;
#define FT_ERROR_START_LIST {
#define FT_ERROR_END_LIST };
#include FT_ERRORS_H
		CAGE_THROW_ERROR(Exception, "unknown freetype error code");
	}

	FT_Library library;
	FT_Face face;

	struct Glyph
	{
		FontHeader::GlyphData data;
		Holder<Image> png;
		uint32 pngX, pngY;
		Glyph() : pngX(0), pngY(0)
		{}
	};

	FontHeader data;

	std::vector<Glyph> glyphs;
	std::vector<real> kerning;
	std::vector<uint32> charsetChars;
	std::vector<uint32> charsetGlyphs;
	Holder<Image> texels;

	vec3 to(const msdfgen::FloatRGB &rgb)
	{
		return vec3(rgb.r, rgb.g, rgb.b);
	}

	msdfgen::Vector2 from(const vec2 &v)
	{
		return msdfgen::Vector2(v[0].value, v[1].value);
	}

	real fontScale;
	real maxOffTop, maxOffBottom;
	static const uint32 border = 6;

	void glyphImage(Glyph &g, msdfgen::Shape &shape)
	{
		shape.normalize();
		msdfgen::edgeColoringSimple(shape, 3.0);
		if (!shape.validate())
			CAGE_THROW_ERROR(Exception, "shape validation failed");
		double l = real::Infinity().value, b = real::Infinity().value, r = -real::Infinity().value, t = -real::Infinity().value;
		shape.bounds(l, b, r, t);

		// generate glyph image
		g.png = newImage();
		g.png->initialize(numeric_cast<uint32>(r - l) + border * 2, numeric_cast<uint32>(t - b) + border * 2, 3);
		msdfgen::Bitmap<msdfgen::FloatRGB> msdf(g.png->width(), g.png->height());
		msdfgen::generateMSDF(msdf, shape, 4.0, 1.0, from(-vec2(l, b) + border));
		for (uint32 y = 0; y < g.png->height(); y++)
			for (uint32 x = 0; x < g.png->width(); x++)
				for (uint32 c = 0; c < 3; c++)
					g.png->value(x, y, c, to(msdf(x, y))[c].value);

		// glyph size compensation for the border
		vec2 ps = vec2(g.png->width(), g.png->height()) - 2 * border;
		vec2 br = border * g.data.size / ps;
		g.data.size += 2 * br;
		g.data.bearing -= br;
	}

	void loadGlyphs()
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "load glyphs");
		data.glyphCount = numeric_cast<uint32>(face->num_glyphs);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "font has " + data.glyphCount + " glyphs");
		glyphs.reserve(data.glyphCount + 10);
		glyphs.resize(data.glyphCount);
		uint32 maxPngW = 0, maxPngH = 0;
		for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
		{
			Glyph &g = glyphs[glyphIndex];
			CALL(FT_Load_Glyph, face, glyphIndex, FT_LOAD_DEFAULT);

			// load glyph metrics
			const FT_Glyph_Metrics &glm = face->glyph->metrics;
			g.data.size[0] = glm.width / 64.0;
			g.data.size[1] = glm.height / 64.0;
			g.data.bearing[0] = glm.horiBearingX / 64.0;
			g.data.bearing[1] = glm.horiBearingY / 64.0;
			g.data.advance = glm.horiAdvance / 64.0;
			g.data.size *= fontScale;
			g.data.bearing *= fontScale;
			g.data.advance *= fontScale;

			// load glyph shape
			msdfgen::Shape shape;
			msdfgen::loadGlyphSlot(shape, face->glyph);
			if (!shape.contours.empty())
				glyphImage(g, shape);

			// update global data
			data.glyphMaxSize = max(data.glyphMaxSize, g.data.size);
			maxOffTop = max(maxOffTop, g.data.size[1] - g.data.bearing[1]);
			maxOffBottom = max(maxOffBottom, g.data.bearing[1]);
			if (g.png)
			{
				maxPngW = max(maxPngW, g.png->width());
				maxPngH = max(maxPngH, g.png->height());
			}
		}
		data.firstLineOffset = maxOffTop;
		data.lineHeight = ((face->height * fontScale / 64.0) + (maxOffTop + maxOffBottom)) / 2;
		CAGE_LOG(SeverityEnum::Note, logComponentName, stringizer() + "units per EM: " + face->units_per_EM);
		CAGE_LOG(SeverityEnum::Note, logComponentName, stringizer() + "(unused) line height (top + bottom): " + (maxOffTop + maxOffBottom));
		CAGE_LOG(SeverityEnum::Note, logComponentName, stringizer() + "(unused) line height (from font): " + (face->height * fontScale / 64.0));
		CAGE_LOG(SeverityEnum::Note, logComponentName, stringizer() + "line height: " + data.lineHeight);
		CAGE_LOG(SeverityEnum::Note, logComponentName, stringizer() + "first line offset: " + data.firstLineOffset);
		CAGE_LOG(SeverityEnum::Note, logComponentName, stringizer() + "max glyph size: " + data.glyphMaxSize);
		CAGE_LOG(SeverityEnum::Note, logComponentName, stringizer() + "max glyph image size: " + maxPngW + "*" + maxPngH);
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
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "font has " + data.charCount + " characters");
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
					CALL(FT_Get_Kerning, face, L, R, FT_KERNING_DEFAULT, &k);
					kerning[L * data.glyphCount + R] = fontScale * (double)k.x / 64.0;
				}
			}
			data.flags |= FontFlags::Kerning;
		}
		else
			CAGE_LOG(SeverityEnum::Info, logComponentName, "font has no kerning");
	}

	msdfgen::Shape cursorShape()
	{
		msdfgen::Shape shape;
		msdfgen::Contour &c = shape.addContour();
		float x = 0.1f / fontScale.value;
		float y0 = (-maxOffBottom / fontScale).value;
		float y1 = (maxOffTop / fontScale).value;
		msdfgen::Point2 points[4] = {
			{0, y0}, {0, y1}, {x, y1}, {x, y0}
		};
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
			CAGE_LOG(SeverityEnum::Warning, logComponentName, stringizer() + "artificially adding return");
			uint32 idx = 0;
			while (idx < charsetChars.size() && charsetChars[idx] < '\n')
				idx++;
			charsetChars.insert(charsetChars.begin() + idx, '\n');
			charsetGlyphs.insert(charsetGlyphs.begin() + idx, (uint32)-1);
			data.charCount++;
		}

		{ // add cursor
			CAGE_LOG(SeverityEnum::Warning, logComponentName, stringizer() + "artificially adding cursor glyph");
			data.glyphCount++;
			glyphs.resize(glyphs.size() + 1);
			Glyph &g = glyphs[glyphs.size() - 1];
			msdfgen::Shape shape = cursorShape();
			glyphImage(g, shape);
			g.data.advance = 0;
			g.data.bearing[0] = -0.1;
			g.data.bearing[1] = data.lineHeight * 3 / 5;
			g.data.size[0] = 0.2;
			g.data.size[1] = data.lineHeight;

			if (!kerning.empty())
			{ // compensate kerning
				std::vector<real> k;
				k.resize(data.glyphCount * data.glyphCount);
				uint32 dgcm1 = data.glyphCount - 1;
				for (uint32 i = 0; i < dgcm1; i++)
					detail::memcpy(&k[i * data.glyphCount], &kerning[i * dgcm1], dgcm1 * sizeof(real));
				std::swap(k, kerning);
			}
		}
	}

	void createAtlasCoordinates()
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "create atlas coordinates");
		RectPackingCreateConfig packingCfg;
		packingCfg.margin = 2;
		Holder<RectPacking> packer = newRectPacking(packingCfg);
		uint32 area = 0;
		uint32 mgs = 0;
		for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
		{
			Glyph &g = glyphs[glyphIndex];
			if (!g.png)
				continue;
			area += g.png->width() * g.png->height();
			mgs = max(mgs, max(g.png->width(), g.png->height()));
			packer->add(glyphIndex, g.png->width(), g.png->height());
		}
		uint32 res = 64;
		while (res < mgs) res *= 2;
		while (res * res < area) res *= 2;
		while (true)
		{
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "trying to pack into resolution " + res + "*" + res);
			if (packer->solve(res, res))
				break;
			res *= 2;
		}
		for (uint32 index = 0, e = packer->count(); index < e; index++)
		{
			uint32 glyphIndex = 0, x = 0, y = 0;
			packer->get(index, glyphIndex, x, y);
			CAGE_ASSERT(glyphIndex < glyphs.size());
			Glyph &g = glyphs[glyphIndex];
			CAGE_ASSERT(x < res);
			CAGE_ASSERT(y < res);
			g.pngX = x;
			g.pngY = y;
			vec2 to = vec2(g.pngX, g.pngY) / res;
			vec2 ts = vec2(g.png->width(), g.png->height()) / res;
			g.data.texUv = vec4(to, ts);
		}
		data.texResolution = ivec2(res);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "texture atlas resolution " + data.texResolution[0] + "*" + data.texResolution[1]);
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
			sizeof(real) * numeric_cast<uint32>(kerning.size()) +
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
			sr.write(bufferCast<char, real>(kerning));
		if (charsetChars.size() > 0)
		{
			sr.write(bufferCast<char, uint32>(charsetChars));
			sr.write(bufferCast<char, uint32>(charsetGlyphs));
		}

		CAGE_ASSERT(h.originalSize == buf.size());
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "buffer size (before compression): " + buf.size());
		Holder<PointerRange<char>> buf2 = compress(buf);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "buffer size (after compression): " + buf2.size());
		h.compressedSize = buf2.size();

		Holder<File> f = writeFile(outputFileName);
		f->write(bufferView(h));
		f->write(buf2);
		f->close();
	}

	void printDebugData()
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "print debug data");

		ConfigString fontPath("cage-asset-processor/font/path", "asset-preview");

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
				fill(string("glyph"), 10) +
				fill(string("tex coord"), 60) +
				fill(string("size"), 30) +
				fill(string("bearing"), 30) +
				string("advance")
			);
			for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
			{
				Glyph &g = glyphs[glyphIndex];
				f->writeLine(
					fill(string(stringizer() + glyphIndex), 10) +
					fill(string(stringizer() + g.data.texUv), 60) +
					fill(string(stringizer() + g.data.size), 30) +
					fill(string(stringizer() + g.data.bearing), 30) +
					string(stringizer() + g.data.advance.value)
				);
			}
		}

		if (configGetBool("cage-asset-processor/font/characters"))
		{ // characters
			FileMode fm(false, true);
			fm.textual = true;
			Holder<File> f = newFile(pathJoin(fontPath, pathReplaceInvalidCharacters(inputName) + ".characters.txt"), fm);
			f->writeLine(
				fill(string("code"), 10) +
				fill(string("char"), 5) +
				string("glyph")
			);
			for (uint32 charIndex = 0; charIndex < data.charCount; charIndex++)
			{
				uint32 c = charsetChars[charIndex];
				char C = c < 256 ? c : ' ';
				f->writeLine(stringizer() +
					fill(string(stringizer() + c), 10) +
					fill(string({ &C, &C + 1 }), 5) +
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
				fill(string("g1"), 5) +
				fill(string("g2"), 5) +
				string("kerning")
			);
			if (kerning.empty())
				f->writeLine("no data");
			else
			{
				uint32 m = data.glyphCount;
				CAGE_ASSERT(kerning.size() == m * m);
				for (uint32 x = 0; x < m; x++)
				{
					for (uint32 y = 0; y < m; y++)
					{
						real k = kerning[x * m + y];
						if (k == 0)
							continue;
						f->writeLine(
							fill(string(stringizer() + x), 5) +
							fill(string(stringizer() + y), 5) +
							string(stringizer() + k)
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
}

void processFont()
{
	writeLine(string("use=") + inputFile);
	if (!inputSpec.empty())
		CAGE_THROW_ERROR(Exception, "input specification must be empty");
	CALL(FT_Init_FreeType, &library);
	CALL(FT_New_Face, library, inputFileName.c_str(), 0, &face);
	if (!FT_IS_SCALABLE(face))
		CAGE_THROW_ERROR(Exception, "font is not scalable");
	uint32 resolution = toUint32(properties("resolution"));
	CALL(FT_Select_Charmap, face, FT_ENCODING_UNICODE);
	CALL(FT_Set_Pixel_Sizes, face, resolution, resolution);
	fontScale = 1.0f / resolution;
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
		writeLine(string() + "asset=" + inputFile);
		writeLine("cage-end");
	}
	catch (...)
	{
		// do nothing
	}
	CALL(FT_Done_FreeType, library);
}
