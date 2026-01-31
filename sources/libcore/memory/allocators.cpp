#include <vector>

#include <cage-core/math.h> // max
#include <cage-core/memoryAllocators.h>
#include <cage-core/memoryArena.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/memoryUtils.h>

namespace cage
{
	namespace
	{
		struct MemoryAllocatorLinearImpl : private Immovable
		{
			explicit MemoryAllocatorLinearImpl(const MemoryAllocatorLinearCreateConfig &config) : config(config) {}

			void updatePointers()
			{
				CAGE_ASSERT(blocks.size() > 0);
				CAGE_ASSERT(index < blocks.size());
				where = (uintPtr)blocks[index].data();
				end = where + blocks[index].size();
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
				CAGE_ASSERT(detail::isPowerOf2(alignment));
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

			void deallocate(void *ptr) { CAGE_THROW_ERROR(Exception, "linear memory allocator does not support individual deallocations"); }

			void flush()
			{
				if (!blocks.empty())
				{
					index = 0;
					updatePointers();
				}
			}

			MemoryArena arena = MemoryArena(this);
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

				sintPtr cnt = 0; // number of allocations in this block
			};

			explicit MemoryAllocatorStreamImpl(const MemoryAllocatorStreamCreateConfig &config) : config(config) {}

			void updatePointers()
			{
				CAGE_ASSERT(blocks.size() > 0);
				CAGE_ASSERT(index < blocks.size());
				where = (uintPtr)blocks[index]->data();
				end = where + blocks[index]->size();
			}

			void nextBuffer()
			{
				if (blocks.empty())
					blocks.push_back(systemMemory().createHolder<Block>(config.blockSize));
				index = (index + 1) % numeric_cast<uint32>(blocks.size());
				if (blocks[index]->cnt != 0)
					blocks.insert(blocks.begin() + index, systemMemory().createHolder<Block>(config.blockSize));
				updatePointers();
			}

			void *allocate(uintPtr size, uintPtr alignment)
			{
				CAGE_ASSERT(detail::isPowerOf2(alignment));
				size = max(size, uintPtr(1));
				alignment = max(alignment, sizeof(uintPtr));
				CAGE_ASSERT(size + alignment + sizeof(uintPtr) <= config.blockSize);
				const uintPtr p = detail::roundUpTo(where + sizeof(uintPtr), alignment);
				const uintPtr e = p + size;
				if (e <= end)
				{
					sintPtr &cnt = blocks[index]->cnt;
					cnt++; // increase number of allocations in this block
					sintPtr **pp = (sintPtr **)(p - sizeof(sintPtr));
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
				sintPtr **pp = (sintPtr **)(p - sizeof(sintPtr));
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

			MemoryArena arena = MemoryArena(this);
			const MemoryAllocatorStreamCreateConfig config;
			std::vector<Holder<Block>> blocks;
			uintPtr where = 0; // pointer inside the current buffer where next allocation should start
			uintPtr end = 0; // pointer at the end of the capacity of the current buffer
			uint32 index = 0; // index of the current buffer
		};

		struct MemoryAllocatorPoolImpl : private Immovable
		{
			static MemoryAllocatorPoolCreateConfig convertConfig(MemoryAllocatorPoolCreateConfig config)
			{
				CAGE_ASSERT(config.itemSize > 0);
				config.itemSize = std::max(config.itemSize, (uintPtr)sizeof(uintPtr));
				CAGE_ASSERT(config.itemAlignment > 0);
				CAGE_ASSERT(detail::isPowerOf2(config.itemAlignment));
				config.itemAlignment = std::max(config.itemAlignment, (uintPtr)sizeof(uintPtr));
				config.itemSize = detail::roundUpTo(config.itemSize, config.itemAlignment);
				CAGE_ASSERT(config.itemSize + config.itemAlignment + sizeof(uintPtr) <= config.blockSize);
				return config;
			}

			explicit MemoryAllocatorPoolImpl(const MemoryAllocatorPoolCreateConfig &config_) : config(convertConfig(config_)) {}

			void addBlock(MemoryBuffer &b)
			{
				char *p = (char *)detail::roundUpTo((uintPtr)b.data(), config.itemAlignment);
				freeList = p;
				char *e = b.data() + b.size() - config.itemSize * 2;
				while (p < e)
				{
					*(void **)p = p + config.itemSize;
					p += config.itemSize;
				}
				*(void **)p = nullptr;
			}

			void newBlock()
			{
				blocks.emplace_back(config.blockSize);
				addBlock(blocks.back());
			}

			void *allocate(uintPtr size, uintPtr alignment)
			{
				CAGE_ASSERT(detail::isPowerOf2(alignment));
				CAGE_ASSERT(size <= config.itemSize);
				CAGE_ASSERT((config.itemAlignment % alignment) == 0);
				if (!freeList)
					newBlock();
				CAGE_ASSERT(freeList);
				void *res = freeList;
				void *prev = *(void **)freeList;
				freeList = prev;
#ifdef CAGE_DEBUG
				*(uintPtr *)res = 0xdeadbeef;
#endif // CAGE_DEBUG
				return res;
			}

			void deallocate(void *ptr)
			{
				*(void **)ptr = freeList;
				freeList = ptr;
			}

			void flush()
			{
				freeList = nullptr;
				for (MemoryBuffer &b : blocks)
					addBlock(b);
			}

			MemoryArena arena = MemoryArena(this);
			const MemoryAllocatorPoolCreateConfig config;
			std::vector<MemoryBuffer> blocks;
			void *freeList = nullptr;
		};
	}

	Holder<MemoryArena> newMemoryAllocatorLinear(const MemoryAllocatorLinearCreateConfig &config)
	{
		Holder<MemoryAllocatorLinearImpl> b = systemMemory().createHolder<MemoryAllocatorLinearImpl>(config);
		return Holder<MemoryArena>(&b->arena, std::move(b));
	}

	Holder<MemoryArena> newMemoryAllocatorStream(const MemoryAllocatorStreamCreateConfig &config)
	{
		Holder<MemoryAllocatorStreamImpl> b = systemMemory().createHolder<MemoryAllocatorStreamImpl>(config);
		return Holder<MemoryArena>(&b->arena, std::move(b));
	}

	Holder<MemoryArena> newMemoryAllocatorPool(const MemoryAllocatorPoolCreateConfig &config)
	{
		Holder<MemoryAllocatorPoolImpl> b = systemMemory().createHolder<MemoryAllocatorPoolImpl>(config);
		return Holder<MemoryArena>(&b->arena, std::move(b));
	}
}
