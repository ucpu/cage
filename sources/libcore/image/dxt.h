#include <cage-core/math.h>

namespace cage
{
	struct Dxt1Block
	{
		uint16 color0;
		uint16 color1;
		uint8 row[4];
	};

	struct Dxt3Block
	{
		uint16 alphaRow[4];
		uint16 color0;
		uint16 color1;
		uint8 row[4];
	};

	struct Dxt5Block
	{
		uint8 alpha[2];
		uint8 alphaBitmap[6];
		uint16 color0;
		uint16 color1;
		uint8 row[4];
	};

	struct TexelBlock
	{
		vec4 texel[4][4];
	};

	TexelBlock dxtDecompress(const Dxt1Block &block);
	TexelBlock dxtDecompress(const Dxt3Block &block);
	TexelBlock dxtDecompress(const Dxt5Block &block);
	Dxt5Block dxtCompress(const TexelBlock &block);
}
