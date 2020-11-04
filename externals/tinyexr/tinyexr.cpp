
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <zlib.h>

#define TINYEXR_IMPLEMENTATION
#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_ZFP 0
#define TINYEXR_USE_THREAD 0
#define TINYEXR_USE_OPENMP 0

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include <tinyexr.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
