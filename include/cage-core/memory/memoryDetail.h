namespace cage
{
	namespace detail
	{
		CAGE_API void *malloca(uintPtr size, uintPtr alignment);
		CAGE_API void freea(void *ptr);

		inline bool isPowerOf2(uintPtr x)
		{
			return x && !(x & (x - 1));
		}

		inline uintPtr addToAlign(uintPtr ptr, uintPtr alignment)
		{
			return (alignment - (ptr % alignment)) % alignment;
		}

		inline uintPtr subtractToAlign(uintPtr ptr, uintPtr alignment)
		{
			return (alignment - addToAlign(ptr, alignment)) % alignment;
		}

		CAGE_API uintPtr compressionBound(uintPtr size);
		CAGE_API uintPtr compress(const void *inputBuffer, uintPtr inputSize, void *outputBuffer, uintPtr outputSize, uint32 quality = 100); // quality is 0 to 100
		CAGE_API uintPtr decompress(const void *inputBuffer, uintPtr inputSize, void *outputBuffer, uintPtr outputSize);
	}
}
