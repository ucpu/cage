#include "bcN.h"

#include <stb_dxt.h>

// inspiration taken from:
// https://github.com/g-truc/gli/blob/8e43030b3e12bb58a4663d85adc5c752f89099c0/gli/core/workaround.hpp
// https://github.com/g-truc/gli/blob/master/gli/core/s3tc.inl

namespace cage
{
	uint16 pack565(const Vec3 &v)
	{
		const Vec3 v3 = Vec3(v * Vec3(31, 63, 31));
		uint16 result = 0;
		result |= (numeric_cast<uint8>(round(v3[0])) & 31u) << 0;
		result |= (numeric_cast<uint8>(round(v3[1])) & 63u) << 5;
		result |= (numeric_cast<uint8>(round(v3[2])) & 31u) << 11;
		return result;
	}

	Vec3 unpack565(uint16 v)
	{
		uint8 x = (v >> 0) & 31u;
		uint8 y = (v >> 5) & 63u;
		uint8 z = (v >> 11) & 31u;
		return Vec3(x, y, z) * Vec3(1.0 / 31, 1.0 / 63, 1.0 / 31);
	}

#ifdef CAGE_DEBUG
	struct PackTest
	{
		PackTest()
		{
			CAGE_ASSERT(pack565(unpack565(0)) == 0);
			CAGE_ASSERT(pack565(unpack565(13)) == 13);
			CAGE_ASSERT(pack565(unpack565(42)) == 42);
			CAGE_ASSERT(pack565(unpack565(456)) == 456);
			CAGE_ASSERT(pack565(unpack565(789)) == 789);
			CAGE_ASSERT(pack565(unpack565(1345)) == 1345);
			CAGE_ASSERT(pack565(unpack565(4679)) == 4679);
		}
	} packTest;
#endif // CAGE_DEBUG

	U8Block F32ToU8Block(const F32Block &in)
	{
		U8Block res;
		for (uint32 y = 0; y < 4; y++)
			for (uint32 x = 0; x < 4; x++)
				for (uint32 c = 0; c < 4; c++)
					res.texel[y][x][c] = numeric_cast<uint8>(in.texel[y][x][c] * 255);
		return res;
	}

	F32Block U8ToF32Block(const U8Block &in)
	{
		F32Block res;
		for (uint32 y = 0; y < 4; y++)
			for (uint32 x = 0; x < 4; x++)
				for (uint32 c = 0; c < 4; c++)
					res.texel[y][x][c] = in.texel[y][x][c] / 255.f;
		return res;
	}

	void bcDecompress(const Bc1Block &in, U8Block &out)
	{
		Vec4 color[4];
		color[0] = Vec4(unpack565(in.color0), 1.0f);
		std::swap(color[0][0], color[0][2]);
		color[1] = Vec4(unpack565(in.color1), 1.0f);
		std::swap(color[1][0], color[1][2]);

		if (in.color0 > in.color1)
		{
			color[2] = (2.0f / 3.0f) * color[0] + (1.0f / 3.0f) * color[1];
			color[3] = (1.0f / 3.0f) * color[0] + (2.0f / 3.0f) * color[1];
		}
		else
		{
			color[2] = (color[0] + color[1]) / 2.0f;
			color[3] = Vec4(0.0f);
		}

		F32Block texelBlock;
		for (uint8 row = 0; row < 4; row++)
		{
			for (uint8 col = 0; col < 4; col++)
			{
				uint8 ColorIndex = (in.row[row] >> (col * 2)) & 0x3;
				texelBlock.texel[row][col] = color[ColorIndex];
			}
		}

		out = F32ToU8Block(texelBlock);
	}

	void bcDecompress(const Bc2Block &in, U8Block &out)
	{
		Vec3 color[4];
		color[0] = Vec3(unpack565(in.color0));
		std::swap(color[0][0], color[0][2]);
		color[1] = Vec3(unpack565(in.color1));
		std::swap(color[1][0], color[1][2]);
		color[2] = (2.0f / 3.0f) * color[0] + (1.0f / 3.0f) * color[1];
		color[3] = (1.0f / 3.0f) * color[0] + (2.0f / 3.0f) * color[1];

		F32Block texelBlock;
		for (uint8 row = 0; row < 4; row++)
		{
			for (uint8 col = 0; col < 4; col++)
			{
				uint8 colorIndex = (in.row[row] >> (col * 2)) & 0x3;
				Real alpha = ((in.alphaRow[row] >> (col * 4)) & 0xF) / 15.0f;
				texelBlock.texel[row][col] = Vec4(color[colorIndex], alpha);
			}
		}

		out = F32ToU8Block(texelBlock);
	}

	void bcDecompress(const Bc3Block &in, U8Block &out)
	{
		Vec3 color[4];
		color[0] = Vec3(unpack565(in.color0));
		std::swap(color[0][0], color[0][2]);
		color[1] = Vec3(unpack565(in.color1));
		std::swap(color[1][0], color[1][2]);
		color[2] = (2.0f / 3.0f) * color[0] + (1.0f / 3.0f) * color[1];
		color[3] = (1.0f / 3.0f) * color[0] + (2.0f / 3.0f) * color[1];

		Real alpha[8];
		alpha[0] = in.alpha[0] / 255.0f;
		alpha[1] = in.alpha[1] / 255.0f;

		if (alpha[0] > alpha[1])
		{
			alpha[2] = (6.0f / 7.0f) * alpha[0] + (1.0f / 7.0f) * alpha[1];
			alpha[3] = (5.0f / 7.0f) * alpha[0] + (2.0f / 7.0f) * alpha[1];
			alpha[4] = (4.0f / 7.0f) * alpha[0] + (3.0f / 7.0f) * alpha[1];
			alpha[5] = (3.0f / 7.0f) * alpha[0] + (4.0f / 7.0f) * alpha[1];
			alpha[6] = (2.0f / 7.0f) * alpha[0] + (5.0f / 7.0f) * alpha[1];
			alpha[7] = (1.0f / 7.0f) * alpha[0] + (6.0f / 7.0f) * alpha[1];
		}
		else
		{
			alpha[2] = (4.0f / 5.0f) * alpha[0] + (1.0f / 5.0f) * alpha[1];
			alpha[3] = (3.0f / 5.0f) * alpha[0] + (2.0f / 5.0f) * alpha[1];
			alpha[4] = (2.0f / 5.0f) * alpha[0] + (3.0f / 5.0f) * alpha[1];
			alpha[5] = (1.0f / 5.0f) * alpha[0] + (4.0f / 5.0f) * alpha[1];
			alpha[6] = 0.0f;
			alpha[7] = 1.0f;
		}

		uint64 bitmap = in.alphaBitmap[0] | (in.alphaBitmap[1] << 8) | (in.alphaBitmap[2] << 16);
		bitmap |= uint64(in.alphaBitmap[3] | (in.alphaBitmap[4] << 8) | (in.alphaBitmap[5] << 16)) << 24;

		F32Block texelBlock;
		for (uint8 row = 0; row < 4; row++)
		{
			for (uint8 col = 0; col < 4; col++)
			{
				uint8 colorIndex = (in.row[row] >> (col * 2)) & 0x3;
				uint8 alphaIndex = (bitmap >> ((row * 4 + col) * 3)) & 0x7;
				texelBlock.texel[row][col] = Vec4(color[colorIndex], alpha[alphaIndex]);
			}
		}

		out = F32ToU8Block(texelBlock);
	}

	void bcCompress(const U8Block &in, Bc3Block &out)
	{
		stb_compress_dxt_block((unsigned char *)&out, (unsigned char *)&in, 1, 2);
	}

	void bcCompress(const U8Block &in, Bc4Block &out)
	{
		uint8 tmp[16];
		for (uint32 y = 0; y < 4; y++)
			for (uint32 x = 0; x < 4; x++)
				tmp[y * 4 + x] = in.texel[y][x][0];
		stb_compress_bc4_block((unsigned char *)&out, tmp);
	}

	void bcCompress(const U8Block &in, Bc5Block &out)
	{
		uint8 tmp[32];
		for (uint32 y = 0; y < 4; y++)
		{
			for (uint32 x = 0; x < 4; x++)
			{
				tmp[(y * 4 + x) * 2 + 0] = in.texel[y][x][0];
				tmp[(y * 4 + x) * 2 + 1] = in.texel[y][x][1];
			}
		}
		stb_compress_bc5_block((unsigned char *)&out, tmp);
	}

	/*
#ifdef CAGE_DEBUG
	struct CompressTest
	{
		CompressTest()
		{
			U8Block a = {};
			for (uint32 y = 0; y < 4; y++)
				for (uint32 x = 0; x < 4; x++)
					for (uint32 i = 0; i < 4; i++)
						a.texel[y][x][i] = (y + x) * 20 + i * 30;
			Bc3Block b;
			bcCompress(a, b);
			U8Block c;
			bcDecompress(b, c);
			uint32 sum = 0;
			for (uint32 y = 0; y < 4; y++)
				for (uint32 x = 0; x < 4; x++)
					for (uint32 i = 0; i < 4; i++)
						sum += abs(sint32(a.texel[y][x][i]) - sint32(c.texel[y][x][i]));
			CAGE_ASSERT(sum < 1000);
		}
	} compressTest;
#endif // CAGE_DEBUG
	*/
}
