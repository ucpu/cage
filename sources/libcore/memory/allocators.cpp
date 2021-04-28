#include <cage-core/memoryAllocators.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/memoryUtils.h>
#include <cage-core/math.h> // max

#include <vector>

namespace cage
{
	namespace
	{
		struct MemoryAllocatorLinearImpl : private Immovable
		{
			explicit MemoryAllocatorLinearImpl(const MemoryAllocatorLinearCreateConfig &config) : arena(this), config(config)
			{}

			void updatePointers()
			{
				CAGE_ASSERT(blocks.size() > 0);
				CAGE_ASSERT(index < blocks.size());
				where = (uintPtr)blocks[index].data();
				end = where + blocks[index].capacity();
			}

			void nextBuffer()
			{
				index++;
				if (blocks.size() <= index)
					blocks.emplace_back(config.blockSize);
				updatePointers();
			}

			void *allocate(uintPtr size, uintPtr alignment)
			{
				size = max(size, uintPtr(1));
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
				if (!blocks.empty())
				{
					index = 0;
					updatePointers();
				}
			}

			MemoryArena arena;
			const MemoryAllocatorLinearCreateConfig config;
			std::vector<MemoryBuffer> blocks;
			uintPtr where = 0; // pointer inside the current buffer where next allocation should start
			uintPtr end = 0; // pointer at the end of the capacity of the current buffer
			uint32 index = m; // index of the current buffer
		};

		struct MemoryAllocatorStreamImpl : private Immovable
		{
			struct Block : public MemoryBuffer
			{
				using MemoryBuffer::MemoryBuffer;

				sint32 cnt = 0;
			};

			explicit MemoryAllocatorStreamImpl(const MemoryAllocatorStreamCreateConfig &config) : arena(this), config(config)
			{}

			void updatePointers()
			{
				CAGE_ASSERT(blocks.size() > 0);
				CAGE_ASSERT(index < blocks.size());
				where = (uintPtr)blocks[index]->data();
				end = where + blocks[index]->capacity();
			}

			void nextBuffer()
			{
				if (blocks.empty())
					blocks.push_back(systemArena().createHolder<Block>(config.blockSize));
				index = (index + 1) % numeric_cast<uint32>(blocks.size());
				if (blocks[index]->cnt != 0)
					blocks.insert(blocks.begin() + index, systemArena().createHolder<Block>(config.blockSize));
				updatePointers();
			}

			void *allocate(uintPtr size, uintPtr alignment)
			{
				size = max(size, uintPtr(1));
				CAGE_ASSERT(size + alignment + sizeof(uintPtr) <= config.blockSize);
				const uintPtr p = detail::roundUpTo(where + sizeof(uintPtr), alignment);
				const uintPtr e = p + size;
				if (e <= end)
				{
					sint32 &cnt = blocks[index]->cnt;
					cnt++; // increase number of allocations in this block
					sint32 **pp = (sint32 **)(p - sizeof(uintPtr));
					*pp = &cnt; // store a pointer to the counter just before the resulting pointer
					where = e; // update position for next allocation
					return (void *)p;
				}
				nextBuffer();
				return allocate(size, alignment);
			}

			void deallocate(void *ptr)
			{
				const uintPtr p = (uintPtr)ptr;
				sint32 **pp = (sint32 **)(p - sizeof(uintPtr));
				(**pp)--; // decrement the number of allocations in this block
				CAGE_ASSERT(**pp >= 0); // detect double deallocations
			}

			void flush()
			{
				index = 0;
				if (!blocks.empty())
				{
					for (const auto &it : blocks)
						it->cnt = 0;
					updatePointers();
				}
			}

			MemoryArena arena;
			const MemoryAllocatorStreamCreateConfig config;
			std::vector<Holder<Block>> blocks;
			uintPtr where = 0; // pointer inside the current buffer where next allocation should start
			uintPtr end = 0; // pointer at the end of the capacity of the current buffer
			uint32 index = 0; // index of the current buffer
		};
	}

	Holder<MemoryArena> newMemoryAllocatorLinear(const MemoryAllocatorLinearCreateConfig &config)
	{
		Holder<MemoryAllocatorLinearImpl> b = systemArena().createHolder<MemoryAllocatorLinearImpl>(config);
		return Holder<MemoryArena>(&b->arena, templates::move(b));
	}

	Holder<MemoryArena> newMemoryAllocatorStream(const MemoryAllocatorStreamCreateConfig &config)
	{
		Holder<MemoryAllocatorStreamImpl> b = systemArena().createHolder<MemoryAllocatorStreamImpl>(config);
		return Holder<MemoryArena>(&b->arena, templates::move(b));
	}
}
