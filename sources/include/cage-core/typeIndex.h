#ifndef guard_typeIndex_h_d65r4h6driolui40yq153
#define guard_typeIndex_h_d65r4h6driolui40yq153

#include "core.h"

namespace cage
{
	namespace privat
	{
		template<class T>
		constexpr StringLiteral typeIndexTypeName() noexcept
		{
			static_assert(std::is_same_v<std::decay_t<T>, T>);
#ifdef _MSC_VER
			return __FUNCSIG__;
#else
			return __PRETTY_FUNCTION__;
#endif // _MSC_VER
		}

		CAGE_CORE_API uint32 typeIndexInitImpl(StringLiteral name, uintPtr size, uintPtr alignment) noexcept;

		template<class T>
		CAGE_FORCE_INLINE uint32 typeIndexInit() noexcept
		{
			return typeIndexInitImpl(privat::typeIndexTypeName<T>(), sizeof(T), alignof(T));
		};

		template<>
		CAGE_FORCE_INLINE uint32 typeIndexInit<void>() noexcept
		{
			return typeIndexInitImpl(privat::typeIndexTypeName<void>(), 0, 0);
		};
	}

	namespace detail
	{
		// returns unique index for each type
		// works well across DLL boundaries
		// the indices may differ between different runs of a program!

		template<class T>
		CAGE_FORCE_INLINE uint32 typeIndex() noexcept
		{
			static const uint32 id = privat::typeIndexInit<T>();
			return id;
		};

		CAGE_CORE_API uintPtr typeSize(uint32 index) noexcept;
		CAGE_CORE_API uintPtr typeAlignment(uint32 index) noexcept;
	}
}

#endif // guard_typeIndex_h_d65r4h6driolui40yq153
