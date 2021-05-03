#include "dxt.h"

#include <stb_dxt.h>

// inspiration taken from:
// https://github.com/g-truc/gli/blob/8e43030b3e12bb58a4663d85adc5c752f89099c0/gli/core/workaround.hpp
// https://github.com/g-truc/gli/blob/master/gli/core/s3tc.inl

namespace cage
{
	uint16 pack565(const vec3 &v)
	{
		const vec3 v3 = vec3(v * vec3(31, 63, 31));
		uint16 result = 0;
		result |= (numeric_cast<uint8>(round(v3[0])) & 31u) << 0;
		result |= (numeric_cast<uint8>(round(v3[1])) & 63u) << 5;
		result |= (numeric_cast<uint8>(round(v3[2])) & 31u) << 11;
		return result;
	}

	vec3 unpack565(uint16 v)
	{
		uint8 x = (v >> 0) & 31u;
		uint8 y = (v >> 5) & 63u;
		uint8 z = (v >> 11) & 31u;
		return vec3(x, y, z) * vec3(1.0 / 31, 1.0 / 63, 1.0 / 31);
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

	TexelBlock dxtDecompress(const Dxt1Block &block)
	{
		vec4 color[4];
		color[0] = vec4(unpack565(block.color0), 1.0f);
		std::swap(color[0][0], color[0][2]);
		color[1] = vec4(unpack565(block.color1), 1.0f);
		std::swap(color[1][0], color[1][2]);

		if (block.color0 > block.color1)
		{
			color[2] = (2.0f / 3.0f) * color[0] + (1.0f / 3.0f) * color[1];
			color[3] = (1.0f / 3.0f) * color[0] + (2.0f / 3.0f) * color[1];
		}
		else
		{
			color[2] = (color[0] + color[1]) / 2.0f;
			color[3] = vec4(0.0f);
		}

		TexelBlock texelBlock;
		for (uint8 row = 0; row < 4; row++)
		{
			for (uint8 col = 0; col < 4; col++)
			{
				uint8 ColorIndex = (block.row[row] >> (col * 2)) & 0x3;
				texelBlock.texel[row][col] = color[ColorIndex];
			}
		}

		return texelBlock;
	}

	TexelBlock dxtDecompress(const Dxt3Block &block)
	{
		vec3 color[4];
		color[0] = vec3(unpack565(block.color0));
		std::swap(color[0][0], color[0][2]);
		color[1] = vec3(unpack565(block.color1));
		std::swap(color[1][0], color[1][2]);
		color[2] = (2.0f / 3.0f) * color[0] + (1.0f / 3.0f) * color[1];
		color[3] = (1.0f / 3.0f) * color[0] + (2.0f / 3.0f) * color[1];

		TexelBlock texelBlock;
		for (uint8 row = 0; row < 4; row++)
		{
			for (uint8 col = 0; col < 4; col++)
			{
				uint8 colorIndex = (block.row[row] >> (col * 2)) & 0x3;
				real alpha = ((block.alphaRow[row] >> (col * 4)) & 0xF) / 15.0f;
				texelBlock.texel[row][col] = vec4(color[colorIndex], alpha);
			}
		}

		return texelBlock;
	}

	TexelBlock dxtDecompress(const Dxt5Block &block)
	{
		vec3 color[4];
		color[0] = vec3(unpack565(block.color0));
		std::swap(color[0][0], color[0][2]);
		color[1] = vec3(unpack565(block.color1));
		std::swap(color[1][0], color[1][2]);
		color[2] = (2.0f / 3.0f) * color[0] + (1.0f / 3.0f) * color[1];
		color[3] = (1.0f / 3.0f) * color[0] + (2.0f / 3.0f) * color[1];

		real alpha[8];
		alpha[0] = block.alpha[0] / 255.0f;
		alpha[1] = block.alpha[1] / 255.0f;

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

		uint64 bitmap = block.alphaBitmap[0] | (block.alphaBitmap[1] << 8) | (block.alphaBitmap[2] << 16);
		bitmap |= uint64(block.alphaBitmap[3] | (block.alphaBitmap[4] << 8) | (block.alphaBitmap[5] << 16)) << 24;

		TexelBlock texelBlock;
		for (uint8 row = 0; row < 4; row++)
		{
			for (uint8 col = 0; col < 4; col++)
			{
				uint8 colorIndex = (block.row[row] >> (col * 2)) & 0x3;
				uint8 alphaIndex = (bitmap >> ((row * 4 + col) * 3)) & 0x7;
				texelBlock.texel[row][col] = vec4(color[colorIndex], alpha[alphaIndex]);
			}
		}

		return texelBlock;
	}

	Dxt5Block dxtCompress(const TexelBlock &block)
	{
		Dxt5Block result;
		uint8 src[16 * 4];
		for (uint32 i = 0; i < 16; i++)
		{
			vec4 v = block.texel[i / 4][i % 4] * 255;
			for (uint32 j = 0; j < 4; j++)
				src[i * 4 + j] = numeric_cast<uint8>(v[j]);
		}
		stb_compress_dxt_block((unsigned char*)&result, src, 1, 2);
		return result;
	}
}
