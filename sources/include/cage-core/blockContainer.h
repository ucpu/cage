#ifndef header_guard_blockCollection_h_m1rt89d6rt4zu
#define header_guard_blockCollection_h_m1rt89d6rt4zu

#include <vector>

#include <cage-core/core.h>

namespace cage
{
	template<class T>
	struct BlockContainer : private Noncopyable
	{
		BlockContainer() = default;

		CAGE_FORCE_INLINE explicit BlockContainer(uint32 maxBlockSize) : blockSize(maxBlockSize) {}

		struct Iterator
		{
			Iterator() = default;

			CAGE_FORCE_INLINE explicit Iterator(const BlockContainer *collection, uint32 index) : collection(collection), index(index) {}

			CAGE_FORCE_INLINE Iterator &operator++()
			{
				index++;
				return *this;
			}

			CAGE_FORCE_INLINE Iterator operator++(int)
			{
				Iterator retval = *this;
				index++;
				return retval;
			}

			CAGE_FORCE_INLINE bool operator==(const Iterator &other) const
			{
				CAGE_ASSERT(collection == other.collection);
				return index == other.index;
			}

			CAGE_FORCE_INLINE bool operator!=(const Iterator &other) const
			{
				CAGE_ASSERT(collection == other.collection);
				return index != other.index;
			}

			CAGE_FORCE_INLINE PointerRange<T> operator->() const { return const_cast<BlockContainer *>(collection)->blocks.at(index); }
			CAGE_FORCE_INLINE PointerRange<T> operator*() const { return const_cast<BlockContainer *>(collection)->blocks.at(index); }

		private:
			const BlockContainer *collection = nullptr;
			uint32 index = m;
		};

		CAGE_FORCE_INLINE Iterator begin() const { return Iterator(this, 0); }
		CAGE_FORCE_INLINE Iterator end() const { return Iterator(this, numeric_cast<uint32>(blocks.size())); }
		CAGE_FORCE_INLINE bool empty() const { return size_ == 0; }
		CAGE_FORCE_INLINE uint32 size() const { return size_; }

		CAGE_FORCE_INLINE void push_back(const T &val)
		{
			inserting().push_back(val);
			size_++;
		}

		CAGE_FORCE_INLINE void push_back(T &&val)
		{
			inserting().push_back(std::move(val));
			size_++;
		}

		CAGE_FORCE_INLINE void clear()
		{
			blocks.clear();
			size_ = 0;
		}

	private:
		std::vector<std::vector<T>> blocks;
		uint32 size_ = 0;
		uint32 blockSize = 200;

		CAGE_FORCE_INLINE std::vector<T> &inserting()
		{
			if ((size_ % blockSize) == 0)
			{
				blocks.push_back({});
				if (size_)
					blocks.back().reserve(blockSize);
			}
			return blocks.back();
		}
	};
}

#endif
