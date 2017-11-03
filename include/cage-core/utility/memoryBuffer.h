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

		void reallocate(uintPtr size); // (re)allocates the buffer, the data is not preserved nor initialized
		void free(); // deallocates the buffer
		void clear(); // fills the buffer with zeros
		void resize(uintPtr size); // allows to change the size without actual reallocation; does not initialize any new memory; throws outOfMemoryException when the allocated buffer is too small

		void *data() const;
		uintPtr size() const;
		uintPtr allocated() const;

	private:
		void *data_;
		uintPtr size_;
		uintPtr allocated_;
	};

	namespace detail
	{
		CAGE_API memoryBuffer compress(const memoryBuffer &input, uint32 quality = 100); // quality is 0 to 100
		CAGE_API memoryBuffer decompress(const memoryBuffer &input, uintPtr outputSize);
	}
}

#endif // guard_memoryBuffer_h_10EAF6AFD8624EA4B9E88ECA618A55DD
