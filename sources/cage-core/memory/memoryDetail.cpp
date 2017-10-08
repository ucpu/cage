#include <cstdlib>
#include <atomic>
#include <zlib.h>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>

namespace cage
{
	namespace detail
	{
		void *malloca(uintPtr size, uintPtr alignment)
		{
			CAGE_ASSERT_RUNTIME(isPowerOf2(alignment), "impossible alignment", alignment);
			void *p = detail::systemArena().allocate(size + alignment - 1 + sizeof(void*));
			if (!p)
				return nullptr;
			void *ptr = (void*)((numeric_cast<uintPtr>(p) + sizeof(void*) + alignment - 1) & ~(alignment - 1));
			*((void **)ptr - 1) = p;
			CAGE_ASSERT_RUNTIME(numeric_cast<uintPtr>(ptr) % alignment == 0, ptr, alignment);
			return ptr;
		}

		void freea(void *ptr)
		{
			if (!ptr)
				return;
			void *p = *((void **)ptr - 1);
			detail::systemArena().deallocate(p);
		}

		uintPtr compressionBound(uintPtr size)
		{
			return ::compressBound(numeric_cast<uLong>(size));
		}

		uintPtr compress(const void *inputBuffer, uintPtr inputSize, void *outputBuffer, uintPtr outputSize, uint32 quality)
		{
			CAGE_ASSERT_RUNTIME(quality <= 100, quality);
			uLongf s = numeric_cast<uLongf>(outputSize);
			int level = numeric_cast<int>(quality / 100.f * (Z_BEST_COMPRESSION - Z_BEST_SPEED) + Z_BEST_SPEED);
			int res = ::compress2((Bytef*)outputBuffer, &s, (const Bytef*)inputBuffer, numeric_cast<uLong>(inputSize), level);
			switch (res)
			{
			case Z_OK:
				return s;
			case Z_MEM_ERROR: // some allocation failed
				CAGE_THROW_ERROR(exception, "compression failed with allocation error");
			case Z_BUF_ERROR: // output buffer was too small
				CAGE_THROW_ERROR(outOfMemoryException, "output buffer for compression is too small", 0);
			case Z_STREAM_ERROR:
				CAGE_THROW_CRITICAL(exception, "invalid compression level");
			default:
				CAGE_THROW_CRITICAL(exception, "compression failed with unknown status");
			}
		}

		uintPtr decompress(const void *inputBuffer, uintPtr inputSize, void *outputBuffer, uintPtr outputSize)
		{
			uLongf s = numeric_cast<uLongf>(outputSize);
			int res = ::uncompress((Bytef*)outputBuffer, &s, (const Bytef*)inputBuffer, numeric_cast<uLong>(inputSize));
			switch (res)
			{
			case Z_OK:
				return s;
			case Z_MEM_ERROR: // some allocation failed
				CAGE_THROW_ERROR(exception, "decompression failed with allocation error");
			case Z_BUF_ERROR: // output buffer was too small
				CAGE_THROW_ERROR(outOfMemoryException, "output buffer for decompression is too small", 0);
			case Z_DATA_ERROR:
				CAGE_THROW_ERROR(exception, "input buffer for decompression is corrupted");
			default:
				CAGE_THROW_CRITICAL(exception, "decompression failed with unknown status");
			}
		}

		namespace
		{
			class memory1Impl
			{
			public:
				memory1Impl() : allocations(0)
				{}

				~memory1Impl()
				{
					try
					{
						if (allocations != 0)
							CAGE_THROW_CRITICAL(exception, "memory corruption - memory leak detected");
					}
					catch (...)
					{}
				}

				void *allocate(uintPtr size)
				{
					void *tmp = ::malloc(size);
					if (!tmp)
						CAGE_THROW_ERROR(outOfMemoryException, "out of memory", size);
					allocations++;
					return tmp;
				}

				void deallocate(void *ptr)
				{
					if (!ptr)
						return;
					::free(ptr);
					if (allocations-- == 0)
						CAGE_THROW_CRITICAL(exception, "memory corruption - double deallocation detected");
				}

				void flush()
				{
					CAGE_THROW_CRITICAL(exception, "invalid operation - deallocate must be used");
				}

				std::atomic<uint32> allocations;
			};
		}

		memoryArena &systemArena()
		{
			static memory1Impl *impl = new memory1Impl(); // intentionally left to leak
			static memoryArena *arena = new memoryArena(impl);
			return *arena;
		}
	}
}
