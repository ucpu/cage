#ifndef guard_stringLiteral_h_fgh4r1se6ft4hj
#define guard_stringLiteral_h_fgh4r1se6ft4hj

#include <cage-core/core.h>

namespace cage
{
	// use as template parameter
	template<uint32 N>
	struct StringLiteral
	{
		consteval StringLiteral(const char (&str)[N])
		{
			static_assert(N > 0);
			detail::memcpy(value, str, N);
		}
		char value[N];
	};
}

#endif // guard_stringLiteral_h_fgh4r1se6ft4hj
