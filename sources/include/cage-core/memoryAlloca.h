#ifndef guard_memoryAlloca_h_xc1drft564g57i
#define guard_memoryAlloca_h_xc1drft564g57i

#ifdef CAGE_SYSTEM_WINDOWS
	#include <malloc.h>
	#define CAGE_ALLOCA(SIZE) _alloca(SIZE)
#else
	#include <alloca.h>
	#define CAGE_ALLOCA(SIZE) alloca(SIZE)
#endif

#endif // guard_memoryAlloca_h_xc1drft564g57i
