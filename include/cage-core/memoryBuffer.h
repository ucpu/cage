#ifndef guard_memoryBuffer_h_10EAF6AFD8624EA4B9E88ECA618A55DD
#define guard_memoryBuffer_h_10EAF6AFD8624EA4B9E88ECA618A55DD

namespace cage
{
	struct CAGE_API memoryBuffer
	{
		memoryBuffer(); // no allocation ctor
		explicit memoryBuffer(uintPtr size, uintPtr capacity = 0);
		memoryBuffer(memoryBuffer &&other) noexcept;
		memoryBuffer &operator = (memoryBuffer &&other) noexcept;
		~memoryBuffer();

		memoryBuffer(const memoryBuffer &) = delete;  // the buffer is non-copyable
		memoryBuffer &operator = (const memoryBuffer &) = delete;

		memoryBuffer copy() const;

		void allocate(uintPtr size, uintPtr cap = 0); // allocates new buffer; sets the size; the data is not preserved nor initialized
		void reserve(uintPtr cap); // allocates new buffer if needed; size and data is preserved; allows the buffer to grow only
		void resizeThrow(uintPtr size); // sets the size; does no allocations or deallocations; does not initialize any new data; throws outOfMemory if capacity is not enough
		void resize(uintPtr size); // reserves enough space and sets the size; does not initialize any new data
		void resizeSmart(uintPtr size); // reserves more than enough space and sets the size; does not initialize any new data
		void shrink(); // reallocates the buffer if needed; size and data is preserved; shrinks the capacity to the size
		void zero(); // fills the buffer (up to current size) with zeros
		void clear(); // sets size to zero; does not deallocate the buffer
		void free(); // deallocates the buffer

		char *data()
		{
			return data_;
		}

		const char *data() const
		{
			return data_;
		}

		uintPtr size() const
		{
			return size_;
		}

		uintPtr capacity() const
		{
			return capacity_;
		}

	private:
		char *data_;
		uintPtr size_;
		uintPtr capacity_;
	};

	namespace detail
	{
		CAGE_API memoryBuffer compress(const memoryBuffer &input);
		CAGE_API memoryBuffer decompress(const memoryBuffer &input, uintPtr outputSize);
	}
}

#endif // guard_memoryBuffer_h_10EAF6AFD8624EA4B9E88ECA618A55DD
