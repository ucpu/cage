namespace cage
{
	namespace detail
	{
		CAGE_API void *memset(void *ptr, int value, uintPtr num);
		CAGE_API void *memcpy(void *destination, const void *source, uintPtr num);
		CAGE_API void *memmove(void *destination, const void *source, uintPtr num);
		CAGE_API int memcmp(const void *ptr1, const void *ptr2, uintPtr num);
		CAGE_API uintPtr strlen(const char *str);
		CAGE_API char *strcpy(char *destination, const char *source);
		CAGE_API char *strcat(char *destination, const char *source);
		CAGE_API int strcmp(const char *str1, const char *str2);
		CAGE_API int toupper(int c);
		CAGE_API int tolower(int c);
	}

	namespace privat
	{
#define GCHL_GENERATE(TYPE) CAGE_API uint32 sprint1(char *s, TYPE value); CAGE_API void sscan1(const char *s, TYPE &value);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, sint8, sint16, sint32, sint64, uint8, uint16, uint32, uint64, float, double));
#undef GCHL_GENERATE
		CAGE_API uint32 sprint1(char *s, const void *value);
		CAGE_API uint32 sprint1(char *s, const char *value);
		CAGE_API uint32 sprint1(char *s, bool value);
	}
}
