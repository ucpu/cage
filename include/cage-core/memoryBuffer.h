#ifndef guard_memoryBuffer_h_10EAF6AFD8624EA4B9E88ECA618A55DD
#define guard_memoryBuffer_h_10EAF6AFD8624EA4B9E88ECA618A55DD

namespace cage
{
	struct CAGE_API memoryBuffer
	{
		memoryBuffer(); // no allocation ctor
		memoryBuffer(uintPtr size);
		memoryBuffer(memoryBuffer &&other) noexcept;
		memoryBuffer &operator = (memoryBuffer &&other) noexcept;
		~memoryBuffer();

		memoryBuffer(const memoryBuffer &) = delete;  // the buffer is non-copyable
		memoryBuffer &operator = (const memoryBuffer &) = delete;

		memoryBuffer copy() const;

		void reserve(uintPtr cap); // allocates new buffer and resets the size to zero, the data is not preserved nor initialized
		void reallocate(uintPtr size); // allocates new buffer and changes the size, the data is not preserved nor initialized
		void free(); // deallocates the buffer
		void zero(); // fills the buffer with zeros
		void resize(uintPtr size); // sets the size without any allocations; does not initialize any new memory; throws outOfMemoryException when the allocated buffer is too small
		void resizeGrow(uintPtr size); // allows to change the size, reallocates (and moves) the buffer when needed
		void resizeGrowSmart(uintPtr size); // same as resizeGrow, but allocates more than is actually needed to hopefully save some memory reallocations in the future

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
		CAGE_API memoryBuffer compress(const memoryBuffer &input); // quality is 0 to 100
		CAGE_API memoryBuffer decompress(const memoryBuffer &input, uintPtr outputSize);
	}
}

#endif // guard_memoryBuffer_h_10EAF6AFD8624EA4B9E88ECA618A55DD
