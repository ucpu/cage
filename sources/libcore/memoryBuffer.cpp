#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/memoryBuffer.h>

namespace cage
{
	memoryBuffer::memoryBuffer() : data_(nullptr), size_(0), capacity_(0)
	{}

	memoryBuffer::memoryBuffer(uintPtr size, uintPtr capacity) : data_(nullptr), size_(0), capacity_(0)
	{
		allocate(size, capacity);
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

	void memoryBuffer::allocate(uintPtr size, uintPtr cap)
	{
		free();
		if (size > cap)
			cap = size;
		data_ = (char*)detail::systemArena().allocate(cap, sizeof(uintPtr));
		capacity_ = cap;
		size_ = size;
	}

	void memoryBuffer::reserve(uintPtr cap)
	{
		if (cap < capacity_)
			return;
		uintPtr s = size_;
		resize(cap);
		size_ = s;
	}

	void memoryBuffer::resizeThrow(uintPtr size)
	{
		if (size > capacity_)
			CAGE_THROW_ERROR(outOfMemory, "size exceeds reserved buffer", size);
		size_ = size;
	}

	void memoryBuffer::resize(uintPtr size)
	{
		if (size <= capacity_)
		{
			size_ = size;
			return;
		}
		memoryBuffer c = templates::move(*this);
		allocate(size);
		detail::memcpy(data_, c.data(), c.size());
	}

	void memoryBuffer::resizeSmart(uintPtr size)
	{
		if (size <= capacity_)
		{
			size_ = size;
			return;
		}
		resize(numeric_cast<uintPtr>(size * 1.5) + 100);
		size_ = size;
	}

	void memoryBuffer::shrink()
	{
		if (capacity_ == size_)
			return;
		CAGE_ASSERT(capacity_ > size_);
		memoryBuffer c = templates::move(*this);
		allocate(c.size());
		detail::memcpy(data_, c.data(), c.size());
	}

	void memoryBuffer::zero()
	{
		detail::memset(data_, 0, size_);
	}

	void memoryBuffer::clear()
	{
		size_ = 0;
	}

	void memoryBuffer::free()
	{
		detail::systemArena().deallocate(data_);
		data_ = nullptr;
		capacity_ = size_ = 0;
	}

	namespace detail
	{
		memoryBuffer compress(const memoryBuffer &input)
		{
			memoryBuffer output(compressionBound(input.size()));
			uintPtr res = compress(input.data(), input.size(), output.data(), output.size());
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
