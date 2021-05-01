#ifndef guard_typeRegistry_h_d65r4h6driolui40yq152
#define guard_typeRegistry_h_d65r4h6driolui40yq152

#include "core.h"

namespace cage
{
	namespace privat
	{
#ifdef CAGE_SYSTEM_WINDOWS
		template<class T> CAGE_API_IMPORT char TypeRegistryBlock;
#else
		template<class T> extern char TypeRegistryBlock;
#endif // CAGE_SYSTEM_WINDOWS

		CAGE_CORE_API uint32 typeRegistryInit(void *ptr);
	}

	namespace detail
	{
		template<class T> CAGE_FORCE_INLINE uint32 typeRegistryId() { static const uint32 value = privat::typeRegistryInit(&privat::TypeRegistryBlock<std::decay_t<T>>); return value; };
	}

	// this macro has to be used inside namespace cage
#define GCHL_REGISTER_TYPE(TYPE) namespace privat { static_assert(std::is_same_v<std::decay_t<TYPE>, TYPE>); template<> CAGE_API_EXPORT char TypeRegistryBlock<TYPE>; }
}

#endif // guard_typeRegistry_h_d65r4h6driolui40yq152
