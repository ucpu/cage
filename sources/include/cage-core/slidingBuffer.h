#ifndef guard_slidingBuffer_h_srhphd54ujz5u4i6dftrz5d
#define guard_slidingBuffer_h_srhphd54ujz5u4i6dftrz5d

#include <vector>

#include <cage-core/core.h>

namespace cage
{
	template<class T>
	class SlidingBuffer : private Noncopyable
	{
	public:
		CAGE_FORCE_INLINE const T *begin() const { return items.data() + offset_; }

		CAGE_FORCE_INLINE T *begin() { return items.data() + offset_; }

		CAGE_FORCE_INLINE const T *end() const { return items.data() + offset_ + size_; }

		CAGE_FORCE_INLINE T *end() { return items.data() + offset_ + size_; }

		CAGE_FORCE_INLINE bool empty() const noexcept { return size_ == 0; }

		CAGE_FORCE_INLINE uintPtr size() const noexcept { return size_; }

		CAGE_FORCE_INLINE const T &front() const
		{
			CAGE_ASSERT(!empty());
			return items[offset_];
		}

		CAGE_FORCE_INLINE T &front()
		{
			CAGE_ASSERT(!empty());
			return items[offset_];
		}

		void push_back(const T &val) { push_back(T(val)); }

		void push_back(T &&val)
		{
			if (offset_ + size_ < items.capacity())
			{ // we have enough space for directly inserting at the end
				if (offset_ + size_ >= items.size())
					items.push_back(std::move(val));
				else
					items[offset_ + size_] = std::move(val);
				size_++;
				return;
			}
			CAGE_ASSERT(offset_ + size_ == items.size());

			if (offset_ * 2 > items.size() + 10)
			{ // we prefer to move existing items to front
				for (uintPtr i = 0; i < size_; i++)
					items[i] = std::move(items[offset_ + i]);
				offset_ = 0;
				items[size_] = std::move(val);
				size_++;
				return;
			}

			// we must reallocate
			items.push_back(std::move(val));
			size_++;
		}

		void pop_front()
		{
			CAGE_ASSERT(!empty());
			items[offset_] = {};
			size_--;
			if (size_ == 0)
				offset_ = 0;
			else
				offset_++;
		}

		void erase(const T *it)
		{
			CAGE_ASSERT(!empty());
			CAGE_ASSERT(it >= items.data() && it < items.data() + items.size());
			const uintPtr index = it - items.data();
			CAGE_ASSERT(index >= offset_ && index < offset_ + size_);
			items[index] = {};

			const uintPtr a = index - offset_;
			const uintPtr b = offset_ + size_ - index - 1;
			CAGE_ASSERT(a + b + 1 == size_);
			if (b <= a)
			{ // shift following items left
				for (uintPtr i = 0; i < b; i++)
					items[index + i] = std::move(items[index + i + 1]);
			}
			else
			{ // shift preceding items right
				for (uintPtr i = a; i; i--)
					items[offset_ + i] = std::move(items[offset_ + i - 1]);
				offset_++;
			}
			size_--;
			if (size_ == 0)
				offset_ = 0;
		}

		void clear()
		{
			offset_ = size_ = 0;
			items.clear();
		}

	protected:
		std::vector<T> items;
		uintPtr offset_ = 0;
		uintPtr size_ = 0;
	};
}

#endif // guard_slidingBuffer_h_srhphd54ujz5u4i6dftrz5d
