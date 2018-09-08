#include <initializer_list>

#include "main.h"
#include <cage-core/math.h>
#include <cage-core/utility/noise.h>
#include <cage-core/utility/png.h>

void testNoise()
{
	CAGE_TESTCASE("noise");

#ifdef CAGE_DEBUG
	sint32 resolution = 256;
#else
	sint32 resolution = 1024;
#endif // CAGE_DEBUG

	{
		CAGE_TESTCASE("value");
		holder<pngImageClass> png = newPngImage();
		png->empty(resolution, resolution, 1, 1);
		uint8 *buffer = (uint8*)png->bufferData();
		for (sint32 y = 0; y < resolution; y++)
		{
			for (sint32 x = 0; x < resolution; x++)
				*buffer++ = numeric_cast<uint8>(noiseValue(42, vec2(x - resolution / 2, y - resolution / 2) * 0.01) * 255);
		}
		png->encodeFile(string() + "value.png");
	}

	{
		CAGE_TESTCASE("clouds");
		for (uint32 oct : {1, 3})
		{
			holder<pngImageClass> png = newPngImage();
			png->empty(resolution, resolution, 1, 1);
			uint8 *buffer = (uint8*)png->bufferData();
			for (sint32 y = 0; y < resolution; y++)
			{
				for (sint32 x = 0; x < resolution; x++)
					*buffer++ = numeric_cast<uint8>(noiseClouds(42, vec2(x - resolution / 2, y - resolution / 2) * 0.01, oct) * 255);
			}
			png->encodeFile(string() + "clouds_" + oct + ".png");
		}
	}

	{
		CAGE_TESTCASE("clouds 3D");
		holder<pngImageClass> png = newPngImage();
		png->empty(resolution, resolution, 1, 1);
		uint8 *buffer = (uint8*)png->bufferData();
		for (sint32 y = 0; y < resolution; y++)
		{
			for (sint32 x = 0; x < resolution; x++)
				*buffer++ += numeric_cast<uint8>(noiseClouds(42, vec3(x - resolution / 2, y - resolution / 2, resolution / 2).normalize() * resolution / 2 * 0.05) * 255);
		}
		png->encodeFile(string() + "clouds_3D.png");
	}

	resolution /= 3;

	{
		CAGE_TESTCASE("cells");
		for (uint32 dst : {0, 1, 2, 3})
		{
			holder<pngImageClass> png = newPngImage();
			png->empty(resolution, resolution, 3, 1);
			uint8 *buffer = (uint8*)png->bufferData();
			for (sint32 y = 0; y < resolution; y++)
			{
				for (sint32 x = 0; x < resolution; x++)
				{
					vec4 n = noiseCell(42, vec2(x - resolution / 2, y - resolution / 2) * 0.05, (noiseDistanceEnum)dst) * 255;
					for (uint32 i = 0; i < 3; i++)
						*buffer++ = numeric_cast<uint8>(n[i]);
				}
			}
			png->encodeFile(string() + "cells_" + dst + ".png");
		}
	}

	{
		CAGE_TESTCASE("cells 3D");
		holder<pngImageClass> png = newPngImage();
		png->empty(resolution, resolution, 1, 1);
		uint8 *buffer = (uint8*)png->bufferData();
		for (sint32 y = 0; y < resolution; y++)
		{
			for (sint32 x = 0; x < resolution; x++)
				*buffer++ += numeric_cast<uint8>(noiseCell(42, vec3(x - resolution / 2, y - resolution / 2, resolution / 2).normalize() * resolution / 2 * 0.05)[2] * 255);
		}
		png->encodeFile(string() + "cells_3D.png");
	}
}
