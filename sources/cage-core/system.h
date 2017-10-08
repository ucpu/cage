#ifdef CAGE_SYSTEM_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif
#include <windows.h>
#include <intrin.h> // __debugbreak
#include <io.h>
#include <fcntl.h>

#else

#define _FILE_OFFSET_BITS 64
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>

#endif

#include <exception>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <time.h>
