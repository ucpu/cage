#ifndef guard_memoryInplaceBuffer_h_w9a8erft7gzkl
#define guard_memoryInplaceBuffer_h_w9a8erft7gzkl

#include <cage-core/core.h>

namespace cage
{
	template<uintPtr Capacity_ = 128>
	struct InplaceBuffer : private Immovable
	{
		static constexpr uintPtr Capacity = Capacity_;
		char data[Capacity];
		uintPtr size = 0;

		CAGE_FORCE_INLINE operator PointerRange<char>() { return { data, data + size }; }
		CAGE_FORCE_INLINE operator PointerRange<const char>() const { return { data, data + size }; }
	};
}

#endif // guard_memoryInplaceBuffer_h_w9a8erft7gzkl
