#include <algorithm>
#include <unordered_map>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <msdfgen/ext/import-font.h>
#include <msdfgen/msdfgen.h>

#include "processor.h"

#include <cage-core/endianness.h>
#include <cage-core/enumerate.h>
#include <cage-core/hashString.h>
#include <cage-core/image.h>
#include <cage-core/imageAlgorithms.h>
#include <cage-core/rectPacking.h>
#include <cage-core/tasks.h>
#include <cage-engine/opengl.h>
#include <cage-engine/texture.h>

#define FT_CALL(FNC, ...) \
	if (const FT_Error err = FNC(__VA_ARGS__)) \
	{ \
		CAGE_LOG_THROW(::cage::privat::translateFtErrorCode(err)); \
		CAGE_THROW_ERROR(Exception, "FreeType " #FNC " error"); \
	}

namespace cage
{
	namespace privat
	{
		CAGE_ENGINE_API cage::String translateFtErrorCode(FT_Error code);
	}
}

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
		const uint32 wi = numeric_cast<uint32>(cage::ceil(wf));
		const uint32 hi = numeric_cast<uint32>(cage::ceil(hf));
		msdfgen::Bitmap<float, 3> msdf(wi, hi);
		msdfgen::generateMSDF(msdf, shape, msdfgen::Projection(1, from(Vec2(-l, -b))), 6);

		Holder<Image> png = newImage();
		png->initialize(wi, hi, 3);
		for (uint32 y = 0; y < hi; y++)
			for (uint32 x = 0; x < wi; x++)
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

	void openFont()
	{
		FT_CALL(FT_Init_FreeType, &library);
		FT_CALL(FT_New_Face, library, inputFileName.c_str(), 0, &face);
		if (!FT_IS_SCALABLE(face))
			CAGE_THROW_ERROR(Exception, "font is not scalable");
		FT_CALL(FT_Select_Charmap, face, FT_ENCODING_UNICODE);
	}

	void setSize(uint32 nominalSize)
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "units per EM: " + face->units_per_EM);
		FT_CALL(FT_Set_Pixel_Sizes, face, nominalSize, nominalSize);
		header.nominalSize = nominalSize;
		header.nominalScale = 1.0 / nominalSize / 64;
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
			FT_CALL(FT_Load_Glyph, face, glyphIndex, FT_LOAD_DEFAULT);
			const FT_Glyph_Metrics &glm = face->glyph->metrics;
			g.data.size = Vec2(float(glm.width), float(glm.height)) * header.nominalScale;
			g.data.bearing = Vec2(float(glm.horiBearingX), float(glm.horiBearingY)) * header.nominalScale;
			FT_CALL(msdfgen::readFreetypeOutline, g.shape, &face->glyph->outline);
			if (!g.shape.contours.empty())
				glyphs.push_back(std::move(g));
		}
		msdfgen::destroyFont(handle);
		header.glyphsCount = glyphs.size();
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "non-empty glyphs count: " + header.glyphsCount);
	}

	void computeLineProperties()
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "compute line properties");
		Real maxAscender, minDescender;
		for (char c : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz")
		{
			FT_CALL(FT_Load_Char, face, c, FT_LOAD_DEFAULT);
			const FT_Glyph_Metrics &glm = face->glyph->metrics;
			maxAscender = max(maxAscender, float(glm.horiBearingY) * header.nominalScale);
			minDescender = min(minDescender, (float(glm.horiBearingY) - float(glm.height)) * header.nominalScale);
		}
		header.lineHeight = (maxAscender - minDescender) * 1.2; // slightly extended by default
		header.lineOffset = minDescender * 0.5 - maxAscender;
		CAGE_LOG(SeverityEnum::Note, "assetProcessor", Stringizer() + "line offset: " + header.lineOffset);
		CAGE_LOG(SeverityEnum::Note, "assetProcessor", Stringizer() + "line height: " + header.lineHeight);
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

	void addCursorGlyph()
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "adding cursor glyph");
		Glyph g;
		g.data.glyphId = uint32(-2);
		g.shape = cursorShape();
		glyphs.push_back(std::move(g));
		header.glyphsCount++;
	}

	void createGlyphsImages()
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "create glyphs images");
		tasksRunBlocking<Glyph>("glyphs", glyphs);
	}

	void assignToImages()
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "assign glyphs to images");
		uint32 img = 0;
		uint32 area = 0;
		for (Glyph &g : glyphs)
		{
			CAGE_ASSERT(g.png);
			area += g.png->width() * g.png->height();
			if (area > 1800 * 1800)
			{
				img++;
				area = 0;
			}
			g.data.image = img;
		}
		CAGE_ASSERT(images.empty());
		images.resize(img + 1);
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "images count: " + images.size());
	}

	String textureName(uint32 index)
	{
		return Stringizer() + inputFile + "?" + index;
	}

	uint32 textureHash(uint32 index)
	{
		return HashString(textureName(index));
	}

	void saveTexture(uint32 index, const Image *img)
	{
		TextureHeader data;
		detail::memset(&data, 0, sizeof(TextureHeader));
		data.target = GL_TEXTURE_2D;
		data.resolution = Vec3i(img->width(), img->height(), 1);
		data.channels = img->channels();
		data.filterMin = GL_LINEAR;
		data.filterMag = GL_LINEAR;
		data.filterAniso = 0;
		data.wrapX = GL_CLAMP_TO_EDGE;
		data.wrapY = GL_CLAMP_TO_EDGE;
		data.wrapZ = GL_CLAMP_TO_EDGE;
		data.swizzle[0] = TextureSwizzleEnum::R;
		data.swizzle[1] = TextureSwizzleEnum::G;
		data.swizzle[2] = TextureSwizzleEnum::B;
		data.swizzle[3] = TextureSwizzleEnum::A;
		data.containedLevels = 1;
		data.mipmapLevels = 1;
		data.copyType = GL_UNSIGNED_BYTE;
		data.internalFormat = GL_RGB8;
		data.copyFormat = GL_RGB;

		MemoryBuffer inputBuffer;
		Serializer ser(inputBuffer);
		ser << data;
		ser << Vec3i(img->resolution(), 1);
		ser << numeric_cast<uint32>(img->rawViewU8().size());
		ser.write(bufferCast(img->rawViewU8()));

		AssetHeader h = initializeAssetHeader();
		h.scheme = AssetSchemeIndexTexture;
		h.originalSize = inputBuffer.size();
		Holder<PointerRange<char>> outputBuffer = memoryCompress(inputBuffer);
		h.compressedSize = outputBuffer.size();
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "texture file size: " + h.originalSize + ", compressed size: " + h.compressedSize + ", ratio: " + h.compressedSize / (float)h.originalSize);

		Holder<File> f = writeFile(pathJoin(outputDirectory, Stringizer() + textureHash(index)));
		f->write(bufferView(h));
		f->write(outputBuffer);
		f->close();
	}

	void createImages()
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "pack and compute uv coordinates");

		auto processor = [](uint32 imageIndex)
		{
			uint32 area = 0;
			uint32 mgs = 0;
			uint32 cnt = 0;
			for (const Glyph &g : glyphs)
			{
				CAGE_ASSERT(g.png);
				if (g.data.image != imageIndex)
					continue;
				area += g.png->width() * g.png->height();
				mgs = max(mgs, max(g.png->width(), g.png->height()));
				cnt++;
			}

			Holder<RectPacking> packer = newRectPacking();
			packer->reserve(cnt);
			for (const auto g : enumerate(glyphs))
			{
				if (g->data.image != imageIndex)
					continue;
				packer->insert(PackingRect{ numeric_cast<uint32>(g.index), g->png->width(), g->png->height() });
			}
			CAGE_ASSERT(packer->data().size() == cnt);

			uint32 res = 64;
			while (res < mgs || res * res < area)
				res += 32;

			while (true)
			{
				RectPackingSolveConfig cfg;
				cfg.margin = 1;
				cfg.width = cfg.height = res;
				if (packer->solve(cfg))
					break;
				res += 32;
			}
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "image resolution: " + res);

			Holder<Image> image = newImage();
			image->initialize(res, res, 3);
			for (const auto &it : packer->data())
			{
				const uint32 arrayIndex = it.id, x = it.x, y = it.y;
				CAGE_ASSERT(arrayIndex < glyphs.size());
				Glyph &g = glyphs[arrayIndex];
				CAGE_ASSERT(g.data.image == imageIndex);
				CAGE_ASSERT(x < res);
				CAGE_ASSERT(y < res);
				g.pos = Vec2i(x, y);
				const Vec2 to = Vec2(g.pos) / res;
				const Vec2 ts = Vec2(g.png->width(), g.png->height()) / res;
				g.data.texUv = Vec4(to, ts);
				imageBlit(+g.png, +image, 0, 0, g.pos[0], g.pos[1], g.png->width(), g.png->height());
				cnt--;
			}
			CAGE_ASSERT(cnt == 0);
			saveTexture(imageIndex, +image);
			CAGE_ASSERT(!images[imageIndex]);
			images[imageIndex] = std::move(image);
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "image index: " + imageIndex + ", completed");
		};

		tasksRunBlocking("images", processor, images.size());
	}

	void convertImageNames()
	{
		for (auto &g : glyphs)
			g.data.image = textureHash(g.data.image);
	}

	// https://learn.microsoft.com/en-us/typography/opentype/spec/otff#organization-of-an-opentype-font
	struct TableDirectory
	{
		uint32 sfntVersion;
		uint16 numTables;
		uint16 searchRange;
		uint16 entrySelector;
		uint16 rangeShift;
	};

	void endian(TableDirectory &t)
	{
		t.sfntVersion = endianness::change(t.sfntVersion);
		t.numTables = endianness::change(t.numTables);
		t.searchRange = 0;
		t.entrySelector = 0;
		t.rangeShift = 0;
	}

	struct TableRecord
	{
		uint32 tableTag;
		uint32 checksum;
		uint32 offset;
		uint32 length;

		String tag() const { return String(bufferView(tableTag)); }
	};

	void endian(TableRecord &t)
	{
		t.checksum = endianness::change(t.checksum);
		t.offset = endianness::change(t.offset);
		t.length = endianness::change(t.length);
	}

	Holder<PointerRange<char>> readFontFile()
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "reading font file");

		auto srcBuf = readFile(inputFileName)->readAll();
		Deserializer des(srcBuf);

		TableDirectory dir;
		des >> dir;
		endian(dir);
		if (dir.sfntVersion != 0x00010000 && dir.sfntVersion != 0x4F54544F)
		{
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", "font file not TTF");
			return srcBuf;
		}

		std::vector<TableRecord> trs;
		trs.resize(dir.numTables);
		for (uint32 i = 0; i < dir.numTables; i++)
		{
			TableRecord &r = trs[i];
			des >> r;
			endian(r);
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "record " + r.tag() + ": " + r.length);
		}

		// remove some tables
		std::erase_if(trs, [](const TableRecord &r) { return r.tag() == "glyf"; });

		// read all tables
		std::unordered_map<uint32, PointerRange<const char>> tables;
		for (const TableRecord &r : trs)
		{
			Deserializer d(srcBuf);
			d.subview(r.offset); // skip to position
			tables[r.tableTag] = d.read(r.length);
		}

		// update offsets
		{
			uint32 pos = sizeof(TableDirectory) + trs.size() * sizeof(TableRecord);
			for (TableRecord &r : trs)
			{
				r.offset = pos;
				pos += r.length;
			}
		}

		// serialize modified file
		dir.numTables = trs.size();
		MemoryBuffer dstBuf;
		Serializer ser(dstBuf);
		endian(dir);
		ser << dir;
		for (TableRecord &r : trs)
		{
			endian(r);
			ser << r;
		}
		for (const TableRecord &r : trs)
			ser.write(tables[r.tableTag]);
		return std::move(dstBuf);
	}

	void exportData()
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "exporting data");
		CAGE_ASSERT(glyphs.size() == header.glyphsCount);

		auto fl = readFontFile();
		header.ftSize = fl.size();
		header.imagesCount = images.size();

		MemoryBuffer buf;
		Serializer sr(buf);
		sr << header;
		sr.write(fl);
		fl.clear();
		for (const Glyph &g : glyphs)
			sr << g.data;

		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "buffer size (before compression): " + buf.size());
		Holder<PointerRange<char>> buf2 = memoryCompress(buf);
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "buffer size (after compression): " + buf2.size());

		AssetHeader h = initializeAssetHeader();
		h.originalSize = buf.size();
		h.compressedSize = buf2.size();

		Holder<File> f = writeFile(outputFileName);
		f->write(bufferView(h));
		f->write(buf2);
		f->close();

		for (uint32 i = 0; i < header.imagesCount; i++)
			writeLine(String("ref=") + textureName(i));
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
}

void processFont()
{
	writeLine(String("use=") + inputFile);
	if (inputSpec.empty())
	{
		openFont();
		setSize(40);
		loadGlyphs();
		computeLineProperties();
		addCursorGlyph();
		createGlyphsImages();
		assignToImages();
		createImages();
		convertImageNames();
		exportData();
		printDebugData();
		FT_CALL(FT_Done_FreeType, library);
	}
	else if (!isInteger(inputSpec))
	{
		CAGE_THROW_ERROR(Exception, "invalid input specification");
	}
	clearAll();
}

void analyzeFont()
{
	try
	{
		openFont();
		FT_CALL(FT_Done_FreeType, library);
		writeLine("cage-begin");
		writeLine("scheme=font");
		writeLine(String() + "asset=" + inputFile);
		writeLine("cage-end");
	}
	catch (...)
	{
		// do nothing
	}
}
