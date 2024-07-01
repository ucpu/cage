#ifndef guard_profiling_h_q9n1a8t4l3
#define guard_profiling_h_q9n1a8t4l3

#include <cage-core/core.h>

#ifdef CAGE_PROFILING_ENABLED
	#define GCHL_PROFILING_API CAGE_CORE_API
	#define GCHL_PROFILING_BODY(BODY)
#else
	#define GCHL_PROFILING_API CAGE_FORCE_INLINE
	#define GCHL_PROFILING_BODY(BODY) \
		{ \
			BODY \
		}
#endif

namespace cage
{
	struct ProfilingFrameTag
	{};

	struct ProfilingEvent
	{
#ifdef CAGE_PROFILING_ENABLED
		String data;
		StringPointer name;
		uint64 startTime = m;
		bool framing = false;
#endif
		GCHL_PROFILING_API void set(const String &data) GCHL_PROFILING_BODY(;);
	};

	[[nodiscard]] GCHL_PROFILING_API ProfilingEvent profilingEventBegin(StringPointer name) GCHL_PROFILING_BODY(return {};);
	[[nodiscard]] GCHL_PROFILING_API ProfilingEvent profilingEventBegin(StringPointer name, ProfilingFrameTag) GCHL_PROFILING_BODY(return {};);
	GCHL_PROFILING_API void profilingEventEnd(ProfilingEvent &ev) GCHL_PROFILING_BODY(;);

	struct ProfilingScope : private Noncopyable
	{
		[[nodiscard]] GCHL_PROFILING_API ProfilingScope() GCHL_PROFILING_BODY(;); // empty/invalid scope
		[[nodiscard]] GCHL_PROFILING_API explicit ProfilingScope(StringPointer name) GCHL_PROFILING_BODY(;);
		[[nodiscard]] GCHL_PROFILING_API explicit ProfilingScope(StringPointer name, ProfilingFrameTag) GCHL_PROFILING_BODY(;);
		GCHL_PROFILING_API ProfilingScope(ProfilingScope &&other) GCHL_PROFILING_BODY(;);
		GCHL_PROFILING_API ProfilingScope &operator=(ProfilingScope &&other) GCHL_PROFILING_BODY(return *this;);
		GCHL_PROFILING_API ~ProfilingScope() GCHL_PROFILING_BODY(;);
		GCHL_PROFILING_API void set(const String &data) GCHL_PROFILING_BODY(;);

	private:
#ifdef CAGE_PROFILING_ENABLED
		ProfilingEvent event;
#endif
	};
}

#undef GCHL_PROFILING_API
#undef GCHL_PROFILING_BODY

#endif // guard_profiling_h_q9n1a8t4l3
