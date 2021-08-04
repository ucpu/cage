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
	enum class ProfilingTypeEnum
	{
		Invalid = 0,
		Event = 1,
		Async = 2,
		Flow = 3,
		Object = 4,
	};

	struct ProfilingEvent
	{
#ifdef CAGE_PROFILING_ENABLED
		string name;
		StringLiteral category;
		uint64 eventId = m;
		ProfilingTypeEnum type = ProfilingTypeEnum::Invalid;
#endif
	};

	[[nodiscard]] GCHL_PROFILING_API ProfilingEvent profilingEventBegin(const string &name, StringLiteral category, ProfilingTypeEnum type = ProfilingTypeEnum::Event) noexcept GCHL_PROFILING_BODY(return {};);
	GCHL_PROFILING_API void profilingEventSnapshot(const ProfilingEvent &ev, const string &jsonParams = {}) noexcept GCHL_PROFILING_BODY(;);
	GCHL_PROFILING_API void profilingEventEnd(const ProfilingEvent &ev, const string &jsonParams = {}) noexcept GCHL_PROFILING_BODY(;);

	GCHL_PROFILING_API void profilingEvent(uint64 duration, const string &name, StringLiteral category, const string &jsonParams = {}) noexcept GCHL_PROFILING_BODY(;);
	GCHL_PROFILING_API void profilingMarker(const string &name, StringLiteral category, bool global = false) noexcept GCHL_PROFILING_BODY(;);

	struct ProfilingScope : private Noncopyable
	{
		[[nodiscard]] GCHL_PROFILING_API ProfilingScope() noexcept GCHL_PROFILING_BODY(;); // empty/invalid scope
		[[nodiscard]] GCHL_PROFILING_API ProfilingScope(const string &name, StringLiteral category, ProfilingTypeEnum type = ProfilingTypeEnum::Event) noexcept GCHL_PROFILING_BODY(;);
		GCHL_PROFILING_API ProfilingScope(ProfilingScope &&other) noexcept GCHL_PROFILING_BODY(;);
		GCHL_PROFILING_API ProfilingScope &operator = (ProfilingScope &&other) noexcept GCHL_PROFILING_BODY(return *this;);
		GCHL_PROFILING_API ~ProfilingScope() noexcept GCHL_PROFILING_BODY(;);
		GCHL_PROFILING_API void snapshot(const string &jsonParams) noexcept GCHL_PROFILING_BODY(;);

	private:
#ifdef CAGE_PROFILING_ENABLED
		ProfilingEvent event;
#endif
	};

	GCHL_PROFILING_API void profilingThreadName(const string &name) noexcept GCHL_PROFILING_BODY(;);
	GCHL_PROFILING_API void profilingThreadOrder(sint32 order) noexcept GCHL_PROFILING_BODY(;);
}

#undef GCHL_PROFILING_API
#undef GCHL_PROFILING_BODY

#endif // guard_profiling_h_q9n1a8t4l3
