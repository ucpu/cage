#include <cage-core/math.h>

namespace cage
{
	struct Bc1Block // r5g6b5a1 dxt1 premultiplied alpha
	{
		uint16 color0;
		uint16 color1;
		uint8 row[4];
	};
	static_assert(sizeof(Bc1Block) == 8);

	struct Bc2Block // r5g6b5a4 dxt3 uncorrelated alpha
	{
		uint16 alphaRow[4];
		uint16 color0;
		uint16 color1;
		uint8 row[4];
	};
	static_assert(sizeof(Bc2Block) == 16);

	struct Bc3Block // r5g6b5a8 dxt5 uncorrelated alpha
	{
		uint8 alpha[2];
		uint8 alphaBitmap[6];
		uint16 color0;
		uint16 color1;
		uint8 row[4];
	};
	static_assert(sizeof(Bc3Block) == 16);

	struct Bc4Block // r8
	{
		uint8 data[8];
	};
	static_assert(sizeof(Bc4Block) == 8);

	struct Bc5Block // r8g8
	{
		uint8 data[16];
	};
	static_assert(sizeof(Bc5Block) == 16);

	struct Bc6Block // r16g16b16
	{
		uint8 data[16];
	};
	static_assert(sizeof(Bc6Block) == 16);

	struct Bc7Block // rgb4-7a0-8
	{
		uint8 data[16];
	};
	static_assert(sizeof(Bc7Block) == 16);

	struct U8Block
	{
		uint8 texel[4][4][4];
	};

	struct F32Block
	{
		Vec4 texel[4][4];
	};

	U8Block F32ToU8Block(const F32Block &in);
	F32Block U8ToF32Block(const U8Block &in);

	void bcDecompress(const Bc1Block &in, U8Block &out);
	void bcDecompress(const Bc2Block &in, U8Block &out);
	void bcDecompress(const Bc3Block &in, U8Block &out);
	void bcDecompress(const Bc4Block &in, U8Block &out);
	void bcDecompress(const Bc5Block &in, U8Block &out);
	void bcDecompress(const Bc6Block &in, F32Block &out);
	void bcDecompress(const Bc7Block &in, U8Block &out);
	void bcCompress(const U8Block &in, Bc1Block &out);
	void bcCompress(const U8Block &in, Bc2Block &out);
	void bcCompress(const U8Block &in, Bc3Block &out);
	void bcCompress(const U8Block &in, Bc4Block &out);
	void bcCompress(const U8Block &in, Bc5Block &out);
	void bcCompress(const F32Block &in, Bc6Block &out);
	void bcCompress(const U8Block &in, Bc7Block &out);
}
