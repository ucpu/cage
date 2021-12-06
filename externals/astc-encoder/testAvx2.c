#include <immintrin.h>

#ifdef __clang__
__attribute__((target("avx2")))
#else
#ifdef __GNUC__
#pragma GCC target("avx2")
#endif
#endif

int main()
{
	int x = _mm_popcnt_u64(42);
	__m128i y = _mm_cvtsi64_si128(42);
	__m256i a = _mm256_set_epi32(1, 2, 3, 4, 5, 6, 7, 8);
	__m256i b = _mm256_add_epi32(a, a);
	__m256i c = _mm256_cmpeq_epi32(a, b);
	return _mm256_extract_epi32(c, 0);
}
