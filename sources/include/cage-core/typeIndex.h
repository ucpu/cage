#ifndef guard_typeIndex_h_d65r4h6driolui40yq153
#define guard_typeIndex_h_d65r4h6driolui40yq153

#include "core.h"

namespace cage
{
	namespace privat
	{
		CAGE_CORE_API uint32 typeIndexInit(const char *name, uintPtr size, uintPtr alignment);

		template<class T>
		constexpr const char *typeIndexTypeName()
		{
			static_assert(std::is_same_v<std::decay_t<T>, T>);
#ifdef _MSC_VER
			return __FUNCSIG__;
#else
			return __PRETTY_FUNCTION__;
#endif // _MSC_VER
		}
	}

	namespace detail
	{
		// returns unique index for each type
		// works well across DLL boundaries
		// the indices may differ between different runs of a program!

		template<class T>
		CAGE_FORCE_INLINE uint32 typeIndex()
		{
			static const uint32 id = privat::typeIndexInit(privat::typeIndexTypeName<T>(), sizeof(T), alignof(T));
			return id;
		};

		template<>
		CAGE_FORCE_INLINE uint32 typeIndex<void>()
		{
			static const uint32 id = privat::typeIndexInit(privat::typeIndexTypeName<void>(), 0, 0);
			return id;
		};

		CAGE_CORE_API uintPtr typeSize(uint32 index);

		template<class T>
		CAGE_FORCE_INLINE uintPtr typeSize()
		{
			return typeSize(typeIndex<T>());
		}

		CAGE_CORE_API uintPtr typeAlignment(uint32 index);

		template<class T>
		CAGE_FORCE_INLINE uintPtr typeAlignment()
		{
			return typeAlignment(typeIndex<T>());
		}
	}
}

#endif // guard_typeIndex_h_d65r4h6driolui40yq153
