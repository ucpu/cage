#include <cage-core/memoryAllocators.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/memoryUtils.h>

#include <vector>

namespace cage
{
	namespace
	{
		struct Block : public MemoryBuffer
		{
			sint32 cnt = 0;
		};

		struct Blocks : private Immovable
		{
			std::vector<Block> blocks;
		};

		struct MemoryAllocatorLinearImpl : private Blocks
		{
			explicit MemoryAllocatorLinearImpl(const MemoryAllocatorLinearCreateConfig &config) : arena(this), config(config)
			{}

			void updatePointers()
			{
				where = (uintPtr)blocks[index].data();
				end = where + blocks[index].capacity();
			}

			void nextBuffer()
			{
				index++;
				if (blocks.size() <= index)
				{
					blocks.resize(index + 1);
					blocks[index].allocate(config.blockSize);
				}
				updatePointers();
			}

			void *allocate(uintPtr size, uintPtr alignment)
			{
				CAGE_ASSERT(size > 0);
				CAGE_ASSERT(size + alignment <= config.blockSize);
				const uintPtr p = detail::roundUpTo(where, alignment);
				const uintPtr e = p + size;
				if (e <= end)
				{
					where = e;
					return (void *)p;
				}
				nextBuffer();
				return allocate(size, alignment);
			}

			void deallocate(void *ptr)
			{
				CAGE_THROW_CRITICAL(Exception, "linear memory allocator does not support individual deallocations");
			}

			void flush()
			{
				index = 0;
				if (!blocks.empty())
					updatePointers();
			}

			MemoryArena arena;
			const MemoryAllocatorLinearCreateConfig config;
			uintPtr where = 0; // pointer inside the current buffer where next allocation should start
			uintPtr end = 0; // pointer at the end of the capacity of the current buffer
			uint32 index = m; // index of the current buffer
		};

		struct MemoryAllocatorStreamImpl : private Blocks
		{
			explicit MemoryAllocatorStreamImpl(const MemoryAllocatorStreamCreateConfig &config) : arena(this)
			{}

			void *allocate(uintPtr size, uintPtr alignment)
			{
				// todo
				CAGE_THROW_CRITICAL(NotImplemented, "stream memory allocator");
			}

			void deallocate(void *ptr)
			{
				// todo
				CAGE_THROW_CRITICAL(NotImplemented, "stream memory allocator");
			}

			void flush()
			{
				// todo
				CAGE_THROW_CRITICAL(NotImplemented, "stream memory allocator");
			}

			MemoryArena arena;
		};
	}

	Holder<MemoryArena> newMemoryAllocatorLinear(const MemoryAllocatorLinearCreateConfig &config)
	{
		Holder<MemoryAllocatorLinearImpl> b = detail::systemArena().createHolder<MemoryAllocatorLinearImpl>(config);
		return Holder<MemoryArena>(&b->arena, std::move(b));
	}

	Holder<MemoryArena> newMemoryAllocatorStream(const MemoryAllocatorStreamCreateConfig &config)
	{
		Holder<MemoryAllocatorStreamImpl> b = detail::systemArena().createHolder<MemoryAllocatorStreamImpl>(config);
		return Holder<MemoryArena>(&b->arena, std::move(b));
	}
}
