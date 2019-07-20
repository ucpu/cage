namespace cage
{
	void graphicsPrepareCreate(const engineCreateConfig &config);
	void graphicsPrepareDestroy();
	void graphicsPrepareEmit(uint64 time);
	void graphicsPrepareTick(uint64 time);

	void graphicsDispatchCreate(const engineCreateConfig &config);
	void graphicsDispatchDestroy();
	void graphicsDispatchInitialize();
	void graphicsDispatchFinalize();
	void graphicsDispatchTick(uint32 &drawCalls, uint32 &drawPrimitives);
	void graphicsDispatchSwap();

	void soundCreate(const engineCreateConfig &config);
	void soundDestroy();
	void soundFinalize();
	void soundEmit(uint64 time);
	void soundTick(uint64 time);

	struct interpolationTimingCorrector
	{
		uint64 operator() (uint64 emit, uint64 dispatch, uint64 step)
		{
			CAGE_ASSERT_RUNTIME(step > 0);
			CAGE_ASSERT_RUNTIME(dispatch > emit);
			uint64 r = dispatch + correction;
			if (r > emit + step)
				correction += sint64(step) / -500;
			else if (r < emit)
				correction += sint64(step) / 500;
			return max(emit, dispatch + correction);
		}

		interpolationTimingCorrector() : correction(-70000)
		{}

		sint64 correction;
	};

	struct clearOnScopeExit
	{
		template<class T>
		clearOnScopeExit(T *&ptr) : ptr((void*&)ptr)
		{}

		~clearOnScopeExit()
		{
			ptr = nullptr;
		}

	private:
		void *&ptr;
	};
}

#include <optick.h>
