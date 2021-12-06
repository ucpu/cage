#include <immintrin.h>

#ifdef __clang__
__attribute__((target("sse4.1")))
#else
#ifdef __GNUC__
#pragma GCC target("sse4.1")
#endif
#endif

int main()
{
	__m128i a = _mm_set_epi32(1, 2, 3, 4);
	__m128i b = _mm_add_epi32(a, a);
	__m128i c = _mm_max_epi32(a, b);
	return _mm_extract_epi32(c, 0) - 2;
}
