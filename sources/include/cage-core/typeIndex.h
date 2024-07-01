#ifndef guard_typeIndex_h_d65r4h6driolui40yq153
#define guard_typeIndex_h_d65r4h6driolui40yq153

#include <cage-core/hashBuffer.h>

namespace cage
{
	namespace privat
	{
		template<class T>
		constexpr PointerRange<const char> typeName()
		{
			static_assert(std::is_same_v<std::decay_t<T>, T>);
			static_assert(std::is_same_v<std::remove_cvref_t<T>, T>);
#ifdef _MSC_VER
			return __FUNCSIG__;
#else
			return __PRETTY_FUNCTION__;
#endif // _MSC_VER
		}

		template<class T>
		consteval uint32 typeHashInit()
		{
			return hashBuffer(typeName<T>());
		};

		CAGE_CORE_API uint32 typeIndexInitImpl(PointerRange<const char> name, uintPtr size, uintPtr alignment);

		template<class T>
		CAGE_FORCE_INLINE uint32 typeIndexInit()
		{
			return typeIndexInitImpl(typeName<T>(), sizeof(T), alignof(T));
		};

		template<>
		CAGE_FORCE_INLINE uint32 typeIndexInit<void>()
		{
			return typeIndexInitImpl(typeName<void>(), 0, 0);
		};
	}

	namespace detail
	{
		// returns unique index for each type
		// works well across DLL boundaries
		// the indices may differ between different runs of a program!
		template<class T>
		CAGE_FORCE_INLINE uint32 typeIndex()
		{
			static const uint32 id = privat::typeIndexInit<T>();
			return id;
		};

		CAGE_CORE_API uintPtr typeSizeByIndex(uint32 index);
		CAGE_CORE_API uintPtr typeAlignmentByIndex(uint32 index);
		CAGE_CORE_API uint32 typeHashByIndex(uint32 index);
		CAGE_CORE_API uint32 typeIndexByHash(uint32 hash);

		// returns unique identifier for each type
		// works well across DLL boundaries
		// the hashes may differ between compilers!
		template<class T>
		consteval uint32 typeHash()
		{
			return privat::typeHashInit<T>();
		};
	}
}

#endif // guard_typeIndex_h_d65r4h6driolui40yq153
