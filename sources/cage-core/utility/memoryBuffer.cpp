#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/utility/memoryBuffer.h>

namespace cage
{
	memoryBuffer::memoryBuffer() : data_(nullptr), size_(0), allocated_(0)
	{}

	memoryBuffer::memoryBuffer(uintPtr size) : data_(nullptr), size_(0), allocated_(0)
	{
		reallocate(size);
	}

	memoryBuffer::memoryBuffer(memoryBuffer &&other) noexcept
	{
		data_ = other.data_;
		size_ = other.size_;
		allocated_ = other.allocated_;
		other.data_ = nullptr;
		other.size_ = 0;
		other.allocated_ = 0;
	}

	memoryBuffer &memoryBuffer::operator = (memoryBuffer &&other) noexcept
	{
		if (&other == this)
			return *this;
		free();
		data_ = other.data_;
		size_ = other.size_;
		allocated_ = other.allocated_;
		other.data_ = nullptr;
		other.size_ = 0;
		other.allocated_ = 0;
		return *this;
	}

	memoryBuffer::~memoryBuffer()
	{
		free();
	}

	memoryBuffer memoryBuffer::copy() const
	{
		memoryBuffer r(size());
		detail::memcpy(r.data(), data(), size());
		return r;
	}

	void memoryBuffer::reallocate(uintPtr size)
	{
		free();
		data_ = detail::systemArena().allocate(size);
		allocated_ = size_ = size;
	}

	void memoryBuffer::free()
	{
		detail::systemArena().deallocate(data_);
		data_ = nullptr;
		allocated_ = size_ = 0;
	}

	void memoryBuffer::clear()
	{
		detail::memset(data_, 0, size_);
	}

	void memoryBuffer::resize(uintPtr size)
	{
		if (size > allocated_)
			CAGE_THROW_ERROR(outOfMemoryException, "size exceeds preallocated buffer", size);
		size_ = size;
	}

	void *memoryBuffer::data() const
	{
		return data_;
	}

	uintPtr memoryBuffer::size() const
	{
		return size_;
	}

	uintPtr memoryBuffer::allocated() const
	{
		return allocated_;
	}

	namespace detail
	{
		memoryBuffer compress(const memoryBuffer &input, uint32 quality)
		{
			memoryBuffer output;
			uintPtr required = compressionBound(input.size());
			if (output.size() < required)
				output.reallocate(required);
			uintPtr res = compress(input.data(), input.size(), output.data(), output.size(), quality);
			output.resize(res);
			return output;
		}

		memoryBuffer decompress(const memoryBuffer &input, uintPtr outputSize)
		{
			memoryBuffer output(outputSize);
			uintPtr res = decompress(input.data(), input.size(), output.data(), output.size());
			output.resize(res);
			return output;
		}
	}
}
