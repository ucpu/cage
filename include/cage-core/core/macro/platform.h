#if defined(_WIN32) || defined(__WIN32__)

#define CAGE_SYSTEM_WINDOWS
#define GCHL_API_EXPORT _declspec(dllexport)
#define GCHL_API_IMPORT _declspec(dllimport)
#if defined (_WIN64)
#define CAGE_ARCHITECTURE_64
#else
#define CAGE_ARCHITECTURE_32
#endif

#elif defined(linux) || defined(__linux) || defined(__linux__)

#define CAGE_SYSTEM_LINUX
#define CAGE_SYSTEM_UNIX

#elif defined(__APPLE__)

#define CAGE_SYSTEM_MAC
#define CAGE_SYSTEM_UNIX

#else

#error This operating system is not supported

#endif

#ifdef CAGE_SYSTEM_UNIX

#define GCHL_API_EXPORT __attribute__((visibility("default")))
#define GCHL_API_IMPORT __attribute__((visibility("default")))
#if __x86_64__ || __ppc64__
#define CAGE_ARCHITECTURE_64
#else
#define CAGE_ARCHITECTURE_32
#endif

#endif

#if defined(__GNUC__) || defined(__clang__)
#define CAGE_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define CAGE_DEPRECATED __declspec(deprecated)
#else
#define CAGE_DEPRECATED
#endif
