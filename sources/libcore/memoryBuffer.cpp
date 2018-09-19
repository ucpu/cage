#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/memoryBuffer.h>

namespace cage
{
	memoryBuffer::memoryBuffer() : data_(nullptr), size_(0), capacity_(0)
	{}

	memoryBuffer::memoryBuffer(uintPtr size) : data_(nullptr), size_(0), capacity_(0)
	{
		reallocate(size);
	}

	memoryBuffer::memoryBuffer(memoryBuffer &&other) noexcept
	{
		data_ = other.data_;
		size_ = other.size_;
		capacity_ = other.capacity_;
		other.data_ = nullptr;
		other.size_ = 0;
		other.capacity_ = 0;
	}

	memoryBuffer &memoryBuffer::operator = (memoryBuffer &&other) noexcept
	{
		if (&other == this)
			return *this;
		free();
		data_ = other.data_;
		size_ = other.size_;
		capacity_ = other.capacity_;
		other.data_ = nullptr;
		other.size_ = 0;
		other.capacity_ = 0;
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

	void memoryBuffer::reserve(uintPtr cap)
	{
		free();
		data_ = (char*)detail::systemArena().allocate(cap);
		capacity_ = cap;
		size_ = 0;
	}

	void memoryBuffer::reallocate(uintPtr size)
	{
		free();
		data_ = (char*)detail::systemArena().allocate(size);
		capacity_ = size_ = size;
	}

	void memoryBuffer::free()
	{
		detail::systemArena().deallocate(data_);
		data_ = nullptr;
		capacity_ = size_ = 0;
	}

	void memoryBuffer::zero()
	{
		detail::memset(data_, 0, size_);
	}

	void memoryBuffer::resize(uintPtr size)
	{
		if (size > capacity_)
			CAGE_THROW_ERROR(outOfMemoryException, "size exceeds preallocated buffer", size);
		size_ = size;
	}

	void memoryBuffer::resizeGrow(uintPtr size)
	{
		if (size <= capacity_)
		{
			size_ = size;
			return;
		}
		memoryBuffer c = copy();
		reallocate(size);
		detail::memcpy(data_, c.data(), c.size());
	}

	void memoryBuffer::resizeGrowSmart(uintPtr size)
	{
		if (size <= capacity_)
		{
			size_ = size;
			return;
		}
		memoryBuffer c = copy();
		reallocate(numeric_cast<uintPtr>(size * 1.5) + 100);
		detail::memcpy(data_, c.data(), c.size());
		size_ = size;
	}

	namespace detail
	{
		memoryBuffer compress(const memoryBuffer &input, uint32 quality)
		{
			memoryBuffer output(compressionBound(input.size()));
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
