#ifndef guard_ringBuffer_h_esr54uguf4olkjhgserdtz
#define guard_ringBuffer_h_esr54uguf4olkjhgserdtz

#include <vector>

#include <cage-core/core.h>

namespace cage
{
	template<class T>
	class RingBuffer : private Noncopyable
	{
	public:
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
			if (size_ == items.size())
			{ // must reallocate
				std::vector<T> repl;
				repl.resize(size_ * 3 / 2 + 10);
				repl.resize(repl.capacity()); // make sure to use all of the allocated capacity
				const uintPtr over = size_ - offset_;
				for (uintPtr i = 0; i < over; i++)
					repl[i] = std::move(items[offset_ + i]);
				for (uintPtr i = 0; i < offset_; i++)
					repl[over + i] = std::move(items[i]);
				std::swap(items, repl);
				offset_ = 0;
			}

			items[(offset_ + size_) % items.size()] = std::move(val);
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
				offset_ = (offset_ + 1) % items.size();
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

#endif // guard_ringBuffer_h_esr54uguf4olkjhgserdtz
