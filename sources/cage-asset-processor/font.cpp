#include <vector>

#include "processor.h"

#include <cage-core/utility/hashString.h>
#include <cage-core/utility/png.h>
#include "utility/binPacking.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BITMAP_H

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
		union
		{
			FT_Glyph glyph;
			FT_BitmapGlyph bitmap;
		};
		FT_BBox box;
		fontHeaderStruct::glyphDataStruct data;

		glyphStruct() : glyph(nullptr)
		{}

		~glyphStruct()
		{
			if (glyph)
				FT_Done_Glyph(glyph);
			glyph = nullptr;
		}
	};

	fontHeaderStruct data;

	std::vector<glyphStruct> glyphs;
	std::vector<sint8> kerning;
	std::vector<uint32> charsetChars;
	std::vector<uint32> charsetGlyphs;
	std::vector<uint8> texPixels;

	void loadGlyphs()
	{
		CAGE_LOG(severityEnum::Info, logComponentName, "load glyphs");
		data.glyphCount = numeric_cast<uint32>(face->num_glyphs);
		data.lineHeight = numeric_cast<uint16>(face->size->metrics.height >> 6);
		data.glyphMaxBearingX = data.glyphMaxBearingY = -30000;
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "font has " + data.glyphCount + " glyphs");
		glyphs.reserve(data.glyphCount + 10);
		glyphs.resize(data.glyphCount);
		for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
		{
			glyphStruct &g = glyphs[glyphIndex];
			CALL(FT_Load_Glyph, face, glyphIndex, FT_LOAD_RENDER);
			CALL(FT_Get_Glyph, face->glyph, &g.glyph);
			g.data.advance = numeric_cast<uint16>(g.glyph->advance.x >> 16);
			FT_Glyph_Get_CBox(g.glyph, FT_GLYPH_BBOX_PIXELS, &g.box);
			g.data.width = numeric_cast<uint16>(g.box.xMax - g.box.xMin);
			g.data.height = numeric_cast<uint16>(g.box.yMax - g.box.yMin);
			CALL(FT_Glyph_To_Bitmap, &g.glyph, FT_RENDER_MODE_NORMAL, nullptr, 1);
			CALL(FT_Bitmap_Convert, library, &g.bitmap->bitmap, &g.bitmap->bitmap, 1);
			g.data.bearX = numeric_cast<sint16>(g.bitmap->left);
			g.data.bearY = numeric_cast<sint16>(g.bitmap->top);
			data.glyphMaxWidth = max(data.glyphMaxWidth, g.data.width);
			data.glyphMaxHeight = max(data.glyphMaxHeight, g.data.height);
			data.glyphMaxBearingX = max(data.glyphMaxBearingX, g.data.bearX);
			data.glyphMaxBearingY = max(data.glyphMaxBearingY, g.data.bearY);
		}
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
			g.data.bearX = -1;
			g.data.bearY = data.lineHeight * 4 / 5;
			g.data.width = 1;
			g.data.height = data.lineHeight;

			if (kerning.size())
			{ // compensate kerning
				std::vector<sint8> k;
				k.resize(data.glyphCount * data.glyphCount, 0);
				uint32 dgcm1 = data.glyphCount - 1;
				for (uint32 i = 0; i < dgcm1; i++)
					detail::memcpy(&k[i * data.glyphCount], &kerning[i * dgcm1], dgcm1);
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
			fontHeaderStruct::glyphDataStruct &g = glyphs[glyphIndex].data;
			area += g.width * g.height;
			packer->add(glyphIndex, g.width, g.height);
		}
		uint32 res[2];
		res[0] = res[1] = 64;
		uint32 tryIndex = 0;
		while (res[0] < data.glyphMaxWidth) res[0] *= 2;
		while (res[1] < data.glyphMaxHeight) res[1] *= 2;
		while (res[0] * res[1] < area) res[(tryIndex++) % 2] *= 2;
		while (true)
		{
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "trying to pack into resolution " + res[0] + "*" + res[1]);
			if (packer->solve(res[0], res[1]))
				break;
			res[(tryIndex++) % 2] *= 2;
		}
		for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
		{
			uint32 id = 0, x = 0, y = 0;
			packer->get(glyphIndex, id, x, y);
			CAGE_ASSERT_RUNTIME(id < glyphs.size(), id, glyphs.size());
			fontHeaderStruct::glyphDataStruct &g = glyphs[id].data;
			CAGE_ASSERT_RUNTIME(x < res[0], "texture x coordinate out of range", x, res[0], glyphIndex, id, g.width, g.height);
			CAGE_ASSERT_RUNTIME(y < res[1], "texture y coordinate out of range", y, res[1], glyphIndex, id, g.width, g.height);
			g.texX = numeric_cast<uint16>(x);
			g.texY = numeric_cast<uint16>(y);
		}
		data.texWidth = numeric_cast<uint16>(res[0]);
		data.texHeight = numeric_cast<uint16>(res[1]);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "texture atlas resolution " + data.texWidth + "*" + data.texHeight);
	}

	void createAtlasPixels()
	{
		CAGE_LOG(severityEnum::Info, logComponentName, "create atlas pixels");
		texPixels.resize(data.texWidth * data.texHeight, 0);
		for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
		{
			glyphStruct &g = glyphs[glyphIndex];
			if (g.data.width == 0 || g.data.height == 0)
				continue;
			if (g.bitmap)
			{
				if (g.bitmap->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
				{
					CAGE_LOG(severityEnum::Note, "exception", string() + "glyph index: " + glyphIndex + " width: " + g.data.width + " height: " + g.data.height + " pixel mode: " + g.bitmap->bitmap.pixel_mode);
					CAGE_THROW_ERROR(exception, "wrong pixel format");
				}
				for (uint32 y = 0; y < g.data.height; y++)
				{
					for (uint32 x = 0; x < g.data.width; x++)
						texPixels[(g.data.texY + y) * data.texWidth + (g.data.texX + x)] = g.bitmap->bitmap.buffer[(g.bitmap->bitmap.rows - y - 1) * g.bitmap->bitmap.pitch + x];
				}
			}
			else
			{
				for (uint32 y = 0; y < g.data.height; y++)
				{
					for (uint32 x = 0; x < g.data.width; x++)
						texPixels[(g.data.texY + y) * data.texWidth + (g.data.texX + x)] = 255;
				}
			}
		}
		data.texSize = data.texWidth * data.texHeight; // no line paddings, 1 byte per texel
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
			sizeof(uint8) * numeric_cast<uint32>(kerning.size()) +
			sizeof(uint32) * numeric_cast<uint32>(charsetChars.size()) +
			sizeof(uint32) * numeric_cast<uint32>(charsetGlyphs.size());

		holder<fileClass> f = newFile(outputFileName, fileMode(false, true));
		f->write(&h, sizeof(h));
		f->write(&data, sizeof(data));
		f->write(&texPixels[0], data.texSize);
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
		if (configGetBool("cage-asset-processor.font.preview"))
		{
			holder<pngImageClass> png = newPngImage();
			png->empty(data.texWidth, data.texHeight, 1);
			detail::memcpy(png->bufferData(), &texPixels[0], data.texWidth * data.texHeight);
			png->encodeFile(pathJoin(configGetString("cage-asset-processor.font.path", "secondary-log"), pathMakeValid(inputName) + ".png"));
		}

		if (configGetBool("cage-asset-processor.font.glyphs"))
		{ // glyphs
			holder<fileClass> f = newFile(pathJoin(configGetString("cage-asset-processor.font.path", "secondary-log"), pathMakeValid(inputName) + ".glyphs.txt"), fileMode(false, true, true));
			f->writeLine(
				string("glyph").fill(10) +
				string("size").fill(10) +
				string("tex coord").fill(10) +
				string("bearing").fill(10) +
				string("advance")
			);
			for (uint32 glyphIndex = 0; glyphIndex < data.glyphCount; glyphIndex++)
			{
				glyphStruct &g = glyphs[glyphIndex];
				f->writeLine(
					string(glyphIndex).fill(10) +
					(string() + g.data.width + "*" + g.data.height).fill(10) +
					(string() + g.data.texX + "*" + g.data.texY).fill(10) +
					(string() + g.data.bearX + "*" + g.data.bearY).fill(10) +
					string(g.data.advance)
				);
			}
		}

		if (configGetBool("cage-asset-processor.font.characters"))
		{ // characters
			holder<fileClass> f = newFile(pathJoin(configGetString("cage-asset-processor.font.path", "secondary-log"), pathMakeValid(inputName) + ".characters.txt"), fileMode(false, true, true));
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
}

void processFont()
{
	writeLine(string("use=") + inputFile);
	if (!inputSpec.isDigitsOnly())
		CAGE_THROW_ERROR(exception, "input specification must be font size (an integer)");
	CALL(FT_Init_FreeType, &library);
	CALL(FT_New_Face, library, inputFileName.c_str(), 0, &face);
	CALL(FT_Select_Charmap, face, FT_ENCODING_UNICODE);
	CALL(FT_Set_Pixel_Sizes, face, 0, inputSpec.toUint32());
	loadGlyphs();
	loadCharset();
	loadKerning();
	addArtificialData();
	createAtlasCoordinates();
	createAtlasPixels();
	exportData();
	printDebugData();
	glyphs.clear();
	CALL(FT_Done_FreeType, library);
}

void analyzeFont()
{
	try
	{
		CALL(FT_Init_FreeType, &library);
		CALL(FT_New_Face, library, inputFileName.c_str(), 0, &face);
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
