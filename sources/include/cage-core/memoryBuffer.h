#ifndef guard_memoryBuffer_h_10EAF6AFD8624EA4B9E88ECA618A55DD
#define guard_memoryBuffer_h_10EAF6AFD8624EA4B9E88ECA618A55DD

#include "core.h"

namespace cage
{
	struct CAGE_CORE_API MemoryBuffer : private Noncopyable
	{
		MemoryBuffer() = default; // no allocation ctor
		explicit MemoryBuffer(uintPtr size, uintPtr capacity = 0);
		MemoryBuffer(MemoryBuffer &&other) noexcept;
		MemoryBuffer &operator = (MemoryBuffer &&other) noexcept;
		~MemoryBuffer();

		MemoryBuffer copy() const;

		void allocate(uintPtr size, uintPtr cap = 0); // allocates new buffer; sets the size; the data is not preserved nor initialized
		void reserve(uintPtr cap); // allocates new buffer if needed; size and data is preserved; allows the buffer to grow only
		void resizeThrow(uintPtr size); // sets the size; does no allocations or deallocations; does not initialize any new data; throws OutOfMemory if capacity is not enough
		void resize(uintPtr size); // reserves enough space and sets the size; does not initialize any new data
		void resizeSmart(uintPtr size); // reserves more than enough space and sets the size; does not initialize any new data
		void shrink(); // reallocates the buffer if needed; size and data is preserved; shrinks the capacity to the size
		void zero(); // fills the buffer (up to current size) with zeros
		void clear(); // sets size to zero; does not deallocate the buffer
		void free(); // deallocates the buffer

		char *data() noexcept
		{
			return data_;
		}

		const char *data() const noexcept
		{
			return data_;
		}

		uintPtr size() const noexcept
		{
			return size_;
		}

		uintPtr capacity() const noexcept
		{
			return capacity_;
		}

		operator PointerRange<char>() noexcept
		{
			return { data_, data_ + size_ };
		}

		operator PointerRange<const char>() const noexcept
		{
			return { data_, data_ + size_ };
		}

		operator Holder<PointerRange<char>>() &&;

	private:
		char *data_ = nullptr;
		uintPtr size_ = 0;
		uintPtr capacity_ = 0;
	};
}

#endif // guard_memoryBuffer_h_10EAF6AFD8624EA4B9E88ECA618A55DD
