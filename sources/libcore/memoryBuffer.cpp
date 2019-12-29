#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/memoryBuffer.h>

namespace cage
{
	MemoryBuffer::MemoryBuffer() : data_(nullptr), size_(0), capacity_(0)
	{}

	MemoryBuffer::MemoryBuffer(uintPtr size, uintPtr capacity) : data_(nullptr), size_(0), capacity_(0)
	{
		allocate(size, capacity);
	}

	MemoryBuffer::MemoryBuffer(MemoryBuffer &&other) noexcept
	{
		data_ = other.data_;
		size_ = other.size_;
		capacity_ = other.capacity_;
		other.data_ = nullptr;
		other.size_ = 0;
		other.capacity_ = 0;
	}

	MemoryBuffer &MemoryBuffer::operator = (MemoryBuffer &&other) noexcept
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

	MemoryBuffer::~MemoryBuffer()
	{
		free();
	}

	MemoryBuffer MemoryBuffer::copy() const
	{
		MemoryBuffer r(size());
		detail::memcpy(r.data(), data(), size());
		return r;
	}

	void MemoryBuffer::allocate(uintPtr size, uintPtr cap)
	{
		free();
		if (size > cap)
			cap = size;
		data_ = (char*)detail::systemArena().allocate(cap, sizeof(uintPtr));
		capacity_ = cap;
		size_ = size;
	}

	void MemoryBuffer::reserve(uintPtr cap)
	{
		if (cap < capacity_)
			return;
		uintPtr s = size_;
		resize(cap);
		size_ = s;
	}

	void MemoryBuffer::resizeThrow(uintPtr size)
	{
		if (size > capacity_)
			CAGE_THROW_ERROR(outOfMemory, "size exceeds reserved buffer", size);
		size_ = size;
	}

	void MemoryBuffer::resize(uintPtr size)
	{
		if (size <= capacity_)
		{
			size_ = size;
			return;
		}
		MemoryBuffer c = templates::move(*this);
		allocate(size);
		detail::memcpy(data_, c.data(), c.size());
	}

	void MemoryBuffer::resizeSmart(uintPtr size)
	{
		if (size <= capacity_)
		{
			size_ = size;
			return;
		}
		resize(numeric_cast<uintPtr>(size * 1.5) + 100);
		size_ = size;
	}

	void MemoryBuffer::shrink()
	{
		if (capacity_ == size_)
			return;
		CAGE_ASSERT(capacity_ > size_);
		MemoryBuffer c = templates::move(*this);
		allocate(c.size());
		detail::memcpy(data_, c.data(), c.size());
	}

	void MemoryBuffer::zero()
	{
		detail::memset(data_, 0, size_);
	}

	void MemoryBuffer::clear()
	{
		size_ = 0;
	}

	void MemoryBuffer::free()
	{
		detail::systemArena().deallocate(data_);
		data_ = nullptr;
		capacity_ = size_ = 0;
	}

	namespace detail
	{
		MemoryBuffer compress(const MemoryBuffer &input)
		{
			MemoryBuffer output(compressionBound(input.size()));
			uintPtr res = compress(input.data(), input.size(), output.data(), output.size());
			output.resize(res);
			return output;
		}

		MemoryBuffer decompress(const MemoryBuffer &input, uintPtr outputSize)
		{
			MemoryBuffer output(outputSize);
			uintPtr res = decompress(input.data(), input.size(), output.data(), output.size());
			output.resize(res);
			return output;
		}
	}
}
