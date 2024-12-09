#include <algorithm>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#define CALL(FNC, ...) \
	{ \
		const int err = FNC(__VA_ARGS__); \
		if (err) \
		{ \
			CAGE_LOG_THROW(translateErrorCode(err)); \
			CAGE_THROW_ERROR(Exception, "FreeType " #FNC " error"); \
		} \
	}
#include <msdfgen/ext/import-font.h>
#include <msdfgen/msdfgen.h>

#include "processor.h"

#include <cage-core/image.h>
#include <cage-core/imageAlgorithms.h>
#include <cage-core/rectPacking.h>
#include <cage-core/tasks.h>

namespace
{
	msdfgen::Vector2 from(Vec2 v)
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
		msdfgen::generateMSDF(msdf, shape, msdfgen::Projection(1.0, from(Vec2(tx, ty))), 6);

		Holder<Image> png = newImage();
		png->initialize(wi, hi, 3);
		for (uint32 y = 0; y < png->height(); y++)
			for (uint32 x = 0; x < png->width(); x++)
				for (uint32 c = 0; c < 3; c++)
					png->value(x, y, c, msdf(x, y)[c]);
		return png;
	}

	struct Glyph
	{
		FontHeader::GlyphData data;
		msdfgen::Shape shape;
		Holder<Image> png;
		Vec2i pos;

		void operator()() { png = glyphImage(shape); }
	};

	FT_Library library;
	FT_Face face;
	FontHeader header;
	std::vector<Glyph> glyphs;
	std::vector<Holder<Image>> images;

	String translateErrorCode(int code)
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
			CAGE_THROW_ERROR(Exception, "unknown freetype error code");
	}

	msdfgen::Shape cursorShape()
	{
		const msdfgen::Point2 points[4] = { { 0, 0 }, { 0, 10 }, { 1, 10 }, { 1, 0 } };
		msdfgen::Shape shape;
		msdfgen::Contour &c = shape.addContour();
		c.addEdge(msdfgen::EdgeHolder(points[0], points[1]));
		c.addEdge(msdfgen::EdgeHolder(points[1], points[2]));
		c.addEdge(msdfgen::EdgeHolder(points[2], points[3]));
		c.addEdge(msdfgen::EdgeHolder(points[3], points[0]));
		return shape;
	}

	void loadGlyphs()
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "load glyphs");
		header.glyphsCount = numeric_cast<uint32>(face->num_glyphs);
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "initial glyphs count: " + header.glyphsCount);
		glyphs.reserve(header.glyphsCount + 10);
		msdfgen::FontHandle *handle = msdfgen::adoptFreetypeFont(face);
		for (uint32 glyphIndex = 0; glyphIndex < header.glyphsCount; glyphIndex++)
		{
			Glyph g;
			g.data.glyphId = glyphIndex;
			CALL(FT_Load_Glyph, face, glyphIndex, FT_LOAD_DEFAULT);
			CALL(msdfgen::readFreetypeOutline, g.shape, &face->glyph->outline);
			if (!g.shape.contours.empty())
				glyphs.push_back(std::move(g));
		}
		msdfgen::destroyFont(handle);
		header.glyphsCount = glyphs.size();
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "non-empty glyphs count: " + header.glyphsCount);
	}

	void createGlyphsImages()
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "create glyphs images");
		tasksRunBlocking<Glyph>("glyphs images", glyphs);
	}

	void createImage()
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "pack and compute uv coordinates");
		Holder<RectPacking> packer = newRectPacking();
		uint32 area = 0;
		uint32 mgs = 0;
		for (const Glyph &g : glyphs)
		{
			CAGE_ASSERT(g.png);
			area += g.png->width() * g.png->height();
			mgs = max(mgs, max(g.png->width(), g.png->height()));
		}
		packer->resize(glyphs.size());
		uint32 index = 0;
		for (const Glyph &g : glyphs)
		{
			packer->data()[index] = PackingRect{ index, g.png->width(), g.png->height() };
			index++;
		}
		uint32 res = 64;
		while (res < mgs)
			res += 32;
		while (res * res < area)
			res += 32;
		while (true)
		{
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "trying to pack into resolution: " + res);
			RectPackingSolveConfig cfg;
			cfg.margin = 1;
			cfg.width = cfg.height = res;
			if (packer->solve(cfg))
				break;
			res += 32;
		}
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "final resolution: " + res);
		for (const auto &it : packer->data())
		{
			uint32 arrayIndex = it.id, x = it.x, y = it.y;
			CAGE_ASSERT(arrayIndex < glyphs.size());
			Glyph &g = glyphs[arrayIndex];
			CAGE_ASSERT(x < res);
			CAGE_ASSERT(y < res);
			g.pos = Vec2i(x, y);
			Vec2 to = Vec2(g.pos) / res;
			Vec2 ts = Vec2(g.png->width(), g.png->height()) / res;
			//to[1] = 1 - to[1] - ts[1];
			g.data.texUv = Vec4(to, ts);
		}

		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "blit glyph images into the atlas");
		Holder<Image> image = newImage();
		image->initialize(res, res, 3);
		for (const Glyph &g : glyphs)
			imageBlit(+g.png, +image, 0, 0, g.pos[0], g.pos[1], g.png->width(), g.png->height());
		//imageVerticalFlip(+image);
		images.push_back(std::move(image));
	}

	void computeLineProperties()
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "compute line properties");
		Real maxAscender, minDescender;
		for (char c : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz")
		{
			CALL(FT_Load_Char, face, c, FT_LOAD_DEFAULT);
			const FT_Glyph_Metrics &glm = face->glyph->metrics;
			maxAscender = max(maxAscender, float(glm.horiBearingY) * header.nominalScale);
			minDescender = min(minDescender, (float(glm.horiBearingY) - float(glm.height)) * header.nominalScale);
		}
		header.lineHeight = maxAscender - minDescender;
		header.lineOffset = minDescender * 0.5 - maxAscender;
		CAGE_LOG(SeverityEnum::Note, "assetProcessor", Stringizer() + "line offset: " + header.lineOffset);
		CAGE_LOG(SeverityEnum::Note, "assetProcessor", Stringizer() + "line height: " + header.lineHeight);
	}

	void exportData()
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "exporting data");
		CAGE_ASSERT(glyphs.size() == header.glyphsCount);

		auto fl = readFile(inputFileName)->readAll();
		header.ftSize = fl.size();
		header.imagesCount = images.size();

		MemoryBuffer buf;
		Serializer sr(buf);
		sr << header;
		{
			sr.write(fl);
			fl.clear();
		}
		for (const auto &it : images)
		{
			fl = it->exportBuffer(".tga"); // will be compressed later anyway
			sr << uint32(fl.size());
			sr.write(fl);
			fl.clear();
		}
		for (const Glyph &g : glyphs)
			sr << g.data;

		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "buffer size (before compression): " + buf.size());
		Holder<PointerRange<char>> buf2 = compress(buf);
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "buffer size (after compression): " + buf2.size());

		AssetHeader h = initializeAssetHeader();
		h.originalSize = buf.size();
		h.compressedSize = buf2.size();

		Holder<File> f = writeFile(outputFileName);
		f->write(bufferView(h));
		f->write(buf2);
		f->close();
	}

	void printDebugData()
	{
		if (!configGetBool("cage-asset-processor/font/preview"))
			return;
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "print debug data");
		const ConfigString fontPath("cage-asset-processor/font/path", "asset-preview");
		uint32 index = 0;
		for (auto &it : images)
		{
			imageVerticalFlip(+it);
			it->exportFile(pathJoin(fontPath, Stringizer() + pathReplaceInvalidCharacters(inputName) + "_" + index + ".png"));
			index++;
		}
	}

	void clearAll()
	{
		header = {};
		glyphs.clear();
		images.clear();
	}

	void setSize(uint32 nominalSize)
	{
		CALL(FT_Set_Pixel_Sizes, face, nominalSize, nominalSize);
		header.nominalSize = nominalSize;
		header.nominalScale = 1.0 / nominalSize / 64;
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
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "units per EM: " + face->units_per_EM);
	setSize(40);
	loadGlyphs();
	createGlyphsImages();
	createImage();
	computeLineProperties();
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
