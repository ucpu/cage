#include <cage-core/memoryUtils.h> // OutOfMemory
#include <cage-core/memoryBuffer.h>

#include <utility> // swap

namespace cage
{
	MemoryBuffer::MemoryBuffer(uintPtr size, uintPtr capacity)
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
		if (size + cap == 0)
			return;
		if (size > cap)
			cap = size;
		data_ = (char*)systemArena().allocate(cap, sizeof(uintPtr));
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
			CAGE_THROW_ERROR(OutOfMemory, "size exceeds reserved buffer", size);
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
		systemArena().deallocate(data_);
		data_ = nullptr;
		capacity_ = size_ = 0;
	}

	MemoryBuffer::operator Holder<PointerRange<char>>() &&
	{
		struct OwnedBufferRange
		{
			MemoryBuffer buff;
			PointerRange<char> range;
		};

		Delegate<void(void *)> d;
		d.bind<MemoryArena, &MemoryArena::destroy<OwnedBufferRange>>(&systemArena());
		OwnedBufferRange *p = systemArena().createObject<OwnedBufferRange>();
		std::swap(*this, p->buff);
		p->range = p->buff;
		Holder<PointerRange<char>> h(&p->range, p, d);
		return h;
	}
}
