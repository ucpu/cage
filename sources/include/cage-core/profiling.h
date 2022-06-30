#ifndef guard_profiling_h_q9n1a8t4l3
#define guard_profiling_h_q9n1a8t4l3

#include "core.h"

#ifdef CAGE_PROFILING_ENABLED
#define GCHL_PROFILING_API CAGE_CORE_API
#define GCHL_PROFILING_BODY(BODY)
#else
#define GCHL_PROFILING_API CAGE_FORCE_INLINE
#define GCHL_PROFILING_BODY(BODY) { BODY }
#endif

namespace cage
{
	struct ProfilingEvent
	{
#ifdef CAGE_PROFILING_ENABLED
		String name;
		StringLiteral category;
		uint64 startTime = m;
#endif
	};

	[[nodiscard]] GCHL_PROFILING_API ProfilingEvent profilingEventBegin(const String &name, StringLiteral category) noexcept GCHL_PROFILING_BODY(return {};);
	GCHL_PROFILING_API void profilingEventEnd(ProfilingEvent &ev) noexcept GCHL_PROFILING_BODY(;);

	GCHL_PROFILING_API void profilingMarker(const String &name, StringLiteral category) noexcept GCHL_PROFILING_BODY(;);

	struct ProfilingScope : private Noncopyable
	{
		[[nodiscard]] GCHL_PROFILING_API ProfilingScope() noexcept GCHL_PROFILING_BODY(;); // empty/invalid scope
		[[nodiscard]] GCHL_PROFILING_API ProfilingScope(const String &name, StringLiteral category) noexcept GCHL_PROFILING_BODY(;);
		GCHL_PROFILING_API ProfilingScope(ProfilingScope &&other) noexcept GCHL_PROFILING_BODY(;);
		GCHL_PROFILING_API ProfilingScope &operator = (ProfilingScope &&other) noexcept GCHL_PROFILING_BODY(return *this;);
		GCHL_PROFILING_API ~ProfilingScope() noexcept GCHL_PROFILING_BODY(;);

	private:
#ifdef CAGE_PROFILING_ENABLED
		ProfilingEvent event;
#endif
	};
}

#undef GCHL_PROFILING_API
#undef GCHL_PROFILING_BODY

#endif // guard_profiling_h_q9n1a8t4l3
