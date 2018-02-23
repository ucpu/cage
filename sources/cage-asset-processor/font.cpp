#include <vector>

#include "processor.h"

#include <cage-core/utility/hashString.h>
#include <cage-core/utility/png.h>
#include "utility/binPacking.h"

#include <msdfgen.h>
#include <msdfgen-ext.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#define CALL(FNC, ...) { int err = FNC(__VA_ARGS__); if (err) { CAGE_LOG(severityEnum::Note, "exception", translateErrorCode(err)); CAGE_THROW_ERROR(codeException, "FreeType " CAGE_STRINGIZE(FNC) " error", err); } }

namespace
{
	const cage::string translateErrorCode(int code)
	{
		switch (code)
#undef __FTERRORS_H__
#define FT_ERRORDEF(E,V,S)  case V: return S;
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       default: CAGE_THROW_ERROR(exception, "unknown freetype error code"); };
#include FT_ERRORS_H
	}

	FT_Library library;
	FT_Face face;

	struct glyphStruct
	{
		fontHeaderStruct::glyphDataStruct data;
		holder<pngImageClass> png;
		uint32 texX, texY;
		glyphStruct() : texX(0), texY(0)
		{}
	};

	fontHeaderStruct data;

	std::vector<glyphStruct> glyphs;
	std::vector<real> kerning;
	std::vector<uint32> charsetChars;
	std::vector<uint32> charsetGlyphs;
	holder<pngImageClass> texels;

	real emScale; // font units -> linear units

	vec3 to(const msdfgen::FloatRGB &rgb)
	{
		return *(vec3*)&rgb;
	}

	msdfgen::Vector2 from(const vec2 &v)
	{
		return *(msdfgen::Vector2*)&v;
	}

	void loadGlyphs()
	{
		CAGE_LOG(severityEnum::Info, logComponentName, "load glyph metrics");
		emScale = 1.f / face->units_per_EM;
		data.glyphCount = numeric_cast<uint32>(face->num_glyphs);
		data.lineHeight = face->size->metrics.height * emScale;
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "font has " + data.glyphCount + " glyphs");
		glyphs.reserve(data.glyphCount + 10);
		glyphs.resize(data.glyphCount);
		for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
		{
			glyphStruct &g = glyphs[glyphIndex];
			CALL(FT_Load_Glyph, face, glyphIndex, FT_LOAD_NO_SCALE);

			// load glyph metrics
			const FT_Glyph_Metrics &glm = face->glyph->metrics;
			g.data.size[0] = glm.width;
			g.data.size[1] = glm.height;
			g.data.bearing[0] = glm.horiBearingX;
			g.data.bearing[1] = glm.horiBearingY;
			g.data.advance = glm.horiAdvance;
			g.data.size *= emScale;
			g.data.bearing *= emScale;
			g.data.advance *= emScale;
			data.glyphMaxSize = max(data.glyphMaxSize, g.data.size);
			data.firstLineOffset = max(data.firstLineOffset, g.data.bearing[1]);

			// load glyph image
			g.png = newPngImage();
			static const real texScale = 32;
			static const uint32 texBorder = 0;
			g.png->empty(numeric_cast<uint32>(g.data.size[0] * texScale) + 2 * texBorder, numeric_cast<uint32>(g.data.size[1] * texScale) + 2 * texBorder, 3);

			msdfgen::Shape shape;
			msdfgen::loadGlyphGlyph(shape, face->glyph);
			shape.normalize();
			msdfgen::edgeColoringSimple(shape, 3.0);
			if (!shape.validate())
				CAGE_THROW_ERROR(exception, "shape validation failed");
			msdfgen::Bitmap<msdfgen::FloatRGB> msdf(g.png->width(), g.png->height());
			msdfgen::generateMSDF(msdf, shape, 0.0, 1.0, from(g.data.bearing));
			for (uint32 y = 0; y < g.png->height(); y++)
				for (uint32 x = 0; x < g.png->width(); x++)
					for (uint32 c = 0; c < 3; c++)
						g.png->value(x, y, c, to(msdf(x, y))[c].value);
		}
		// todo first line offset
	}

	void loadCharset()
	{
		CAGE_LOG(severityEnum::Info, logComponentName, "load charset");
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
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "font has " + data.charCount + " characters");
	}

	void loadKerning()
	{
		CAGE_LOG(severityEnum::Info, logComponentName, "load kerning");
		if (FT_HAS_KERNING(face))
		{
			uint32 nonnull = 0;
			kerning.resize(data.glyphCount * data.glyphCount);
			for (uint32 L = 0; L < data.glyphCount; L++)
			{
				for (uint32 R = 0; R < data.glyphCount; R++)
				{
					FT_Vector k;
					CALL(FT_Get_Kerning, face, L, R, FT_KERNING_DEFAULT, &k);
					sint8 k2 = numeric_cast<sint8>((long)k.x >> 6);
					kerning[L * data.glyphCount + R] = k2;
					if (k2 != 0)
						nonnull++;
				}
			}
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "total non null " + nonnull + " / " + kerning.size() + " kernings");
			data.flags |= fontFlags::Kerning;
		}
		else
			CAGE_LOG(severityEnum::Info, logComponentName, "font has no kerning");
	}

	void addArtificialData()
	{
		CAGE_LOG(severityEnum::Info, logComponentName, "adding artificial data");

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
			CAGE_LOG(severityEnum::Warning, logComponentName, string() + "artificially adding return");
			uint32 idx = 0;
			while (idx < charsetChars.size() && charsetChars[idx] < '\n')
				idx++;
			charsetChars.insert(charsetChars.begin() + idx, '\n');
			charsetGlyphs.insert(charsetGlyphs.begin() + idx, -1);
			data.charCount++;
		}

		{ // add cursor
			CAGE_LOG(severityEnum::Warning, logComponentName, string() + "artificially adding cursor glyph");
			data.glyphCount++;
			glyphs.resize(glyphs.size() + 1);
			glyphStruct &g = glyphs[glyphs.size() - 1];
			g.data.advance = 0;
			g.data.bearing[0] = -0.1;
			g.data.bearing[1] = data.lineHeight * 4 / 5;
			g.data.size[0] = 0.1;
			g.data.size[1] = data.lineHeight;

			if (kerning.size())
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
		CAGE_LOG(severityEnum::Info, logComponentName, "create atlas coordinates");
		uint32 area = 0;
		holder<binPackingClass> packer = newBinPacking();
		for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
		{
			glyphStruct &g = glyphs[glyphIndex];
			area += g.png->width() * g.png->height();
			packer->add(glyphIndex, g.png->width(), g.png->height());
		}
		uint32 res[2] = { 64, 64 };
		uint32 tryIndex = 0;
		while (res[0] < data.glyphMaxSize[0]) res[0] *= 2;
		while (res[1] < data.glyphMaxSize[1]) res[1] *= 2;
		while (res[0] * res[1] < area) res[(tryIndex++) % 2] *= 2;
		while (true)
		{
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "trying to pack into resolution " + res[0] + "*" + res[1]);
			if (packer->solve(res[0], res[1]))
				break;
			res[(tryIndex++) % 2] *= 2;
		}
		vec2 resInv =  1 / vec2(res[0], res[1]);
		for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
		{
			uint32 id = 0, x = 0, y = 0;
			packer->get(glyphIndex, id, x, y);
			CAGE_ASSERT_RUNTIME(id < glyphs.size(), id, glyphs.size());
			glyphStruct &g = glyphs[id];
			CAGE_ASSERT_RUNTIME(x < res[0], "texture x coordinate out of range", x, res[0], glyphIndex, id);
			CAGE_ASSERT_RUNTIME(y < res[1], "texture y coordinate out of range", y, res[1], glyphIndex, id);
			g.texX = x;
			g.texY = y;
			vec2 to = vec2(g.texX, g.texY) * resInv;
			vec2 ts = vec2(g.png->width(), g.png->height()) * resInv;
			g.data.texUv = vec4(to, ts);
		}
		data.texWidth = res[0];
		data.texHeight = res[1];
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "texture atlas resolution " + data.texWidth + "*" + data.texHeight);
	}

	void createAtlasPixels()
	{
		CAGE_LOG(severityEnum::Info, logComponentName, "create atlas pixels");
		texels = newPngImage();
		texels->empty(data.texWidth, data.texHeight, 3);
		for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
		{
			glyphStruct &g = glyphs[glyphIndex];
			if (g.data.size[0] <= 0 || g.data.size[1] <= 0)
				continue;
			pngBlit(g.png.get(), texels.get(), 0, 0, g.texX, g.texY, g.png->width(), g.png->height());
		}
		data.texSize = texels->bufferSize();
	}

	void exportData()
	{
		CAGE_LOG(severityEnum::Info, logComponentName, "export data");

		CAGE_ASSERT_RUNTIME(glyphs.size() == data.glyphCount, glyphs.size(), data.glyphCount);
		CAGE_ASSERT_RUNTIME(charsetChars.size() == data.charCount, charsetChars.size(), data.charCount);
		CAGE_ASSERT_RUNTIME(charsetChars.size() == charsetGlyphs.size(), charsetChars.size(), charsetGlyphs.size());
		CAGE_ASSERT_RUNTIME(kerning.size() == 0 || kerning.size() == data.glyphCount * data.glyphCount, kerning.size(), data.glyphCount * data.glyphCount, data.glyphCount);

		assetHeaderStruct h = initializeAssetHeaderStruct();
		h.originalSize = sizeof(data) + data.texSize +
			data.glyphCount * sizeof(fontHeaderStruct::glyphDataStruct) +
			sizeof(real) * numeric_cast<uint32>(kerning.size()) +
			sizeof(uint32) * numeric_cast<uint32>(charsetChars.size()) +
			sizeof(uint32) * numeric_cast<uint32>(charsetGlyphs.size());

		holder<fileClass> f = newFile(outputFileName, fileMode(false, true));
		f->write(&h, sizeof(h));
		f->write(&data, sizeof(data));
		f->write(texels->bufferData(), texels->bufferSize());
		for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
		{
			glyphStruct &g = glyphs[glyphIndex];
			f->write(&g.data, sizeof(g.data));
		}
		if (kerning.size() > 0)
			f->write(&kerning[0], sizeof(kerning[0]) * kerning.size());
		if (charsetChars.size() > 0)
		{
			f->write(&charsetChars[0], sizeof(charsetChars[0]) * charsetChars.size());
			f->write(&charsetGlyphs[0], sizeof(charsetGlyphs[0]) * charsetGlyphs.size());
		}
		f->close();
	}

	void printDebugData()
	{
		CAGE_LOG(severityEnum::Info, logComponentName, "print debug data");

		if (configGetBool("cage-asset-processor.font.preview"))
		{
			texels->verticalFlip();
			texels->encodeFile(pathJoin(configGetString("cage-asset-processor.font.path", "asset-preview"), pathMakeValid(inputName) + ".png"));
		}

		if (configGetBool("cage-asset-processor.font.glyphs"))
		{ // glyphs
			holder<fileClass> f = newFile(pathJoin(configGetString("cage-asset-processor.font.path", "asset-preview"), pathMakeValid(inputName) + ".glyphs.txt"), fileMode(false, true, true));
			f->writeLine(
				string("glyph").fill(10) +
				string("tex coord").fill(60) +
				string("size").fill(30) +
				string("bearing").fill(30) +
				string("advance")
			);
			for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
			{
				glyphStruct &g = glyphs[glyphIndex];
				f->writeLine(
					string(glyphIndex).fill(10) +
					(string() + g.data.texUv).fill(60) +
					(string() + g.data.size).fill(30) +
					(string() + g.data.bearing).fill(30) +
					string(g.data.advance)
				);
			}
		}

		if (configGetBool("cage-asset-processor.font.characters"))
		{ // characters
			holder<fileClass> f = newFile(pathJoin(configGetString("cage-asset-processor.font.path", "asset-preview"), pathMakeValid(inputName) + ".characters.txt"), fileMode(false, true, true));
			f->writeLine(
				string("code").fill(10) +
				string("char").fill(5) +
				string("glyph")
			);
			for (uint32 charIndex = 0; charIndex < data.charCount; charIndex++)
			{
				uint32 c = charsetChars[charIndex];
				char C = c < 256 ? c : ' ';
				f->writeLine(
					string(c).fill(10) +
					string(&C, 1).fill(5) +
					string(charsetGlyphs[charIndex])
				);
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
		CAGE_THROW_ERROR(exception, "input specification must be empty");
	CALL(FT_Init_FreeType, &library);
	CALL(FT_New_Face, library, inputFileName.c_str(), 0, &face);
	CALL(FT_Select_Charmap, face, FT_ENCODING_UNICODE);
	if (!FT_IS_SCALABLE(face))
		CAGE_THROW_ERROR(exception, "font is not scalable");
	loadGlyphs();
	loadCharset();
	loadKerning();
	//addArtificialData();
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
