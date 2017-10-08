#ifdef CAGE_API
#undef CAGE_API
#endif

#ifdef CAGE_EXPORT
#define CAGE_API GCHL_API_EXPORT
#else
#define CAGE_API GCHL_API_IMPORT
#endif
