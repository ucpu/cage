#include <cage-core/memoryCompression.h>
#include <cage-core/memoryBuffer.h>

#include <zstd.h>

namespace cage
{
	Holder<PointerRange<char>> compress(PointerRange<const char> input, sint32 preference)
	{
		MemoryBuffer result(compressionBound(input.size()));
		PointerRange<char> output = result;
		compress(input, output, preference);
		result.resize(output.size());
		return templates::move(result);
	}

	Holder<PointerRange<char>> decompress(PointerRange<const char> input, uintPtr outputSize)
	{
		MemoryBuffer result(outputSize);
		PointerRange<char> output = result;
		decompress(input, output);
		result.resize(output.size());
		return templates::move(result);
	}

	void compress(PointerRange<const char> input, PointerRange<char> &output, sint32 preference)
	{
		const std::size_t r = ZSTD_compress(output.data(), output.size(), input.data(), input.size(), ZSTD_maxCLevel() * preference / 100);
		if (ZSTD_isError(r))
			CAGE_THROW_ERROR(Exception, Exception::StringLiteral(ZSTD_getErrorName(r)));
		CAGE_ASSERT(r <= output.size());
		output = PointerRange<char>(output.data(), output.data() + r);
	}

	void decompress(PointerRange<const char> input, PointerRange<char> &output)
	{
		const std::size_t r = ZSTD_decompress(output.data(), output.size(), input.data(), input.size());
		if (ZSTD_isError(r))
			CAGE_THROW_ERROR(Exception, Exception::StringLiteral(ZSTD_getErrorName(r)));
		CAGE_ASSERT(r <= output.size());
		output = PointerRange<char>(output.data(), output.data() + r);
	}

	uintPtr compressionBound(uintPtr size)
	{
		const std::size_t r = ZSTD_compressBound(size);
		return r + r / 100 + 100000; // additional capacity allows faster compression
	}
}
