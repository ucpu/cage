namespace cage
{
	void graphicsPrepareCreate(const EngineCreateConfig &config);
	void graphicsPrepareDestroy();
	void graphicsPrepareEmit(uint64 time);
	void graphicsPrepareTick(uint64 time);

	void graphicsDispatchCreate(const EngineCreateConfig &config);
	void graphicsDispatchDestroy();
	void graphicsDispatchInitialize();
	void graphicsDispatchFinalize();
	void graphicsDispatchTick(uint32 &drawCalls, uint32 &drawPrimitives);
	void graphicsDispatchSwap();

	void soundCreate(const EngineCreateConfig &config);
	void soundDestroy();
	void soundFinalize();
	void soundEmit(uint64 time);
	void soundTick(uint64 time);

	struct InterpolationTimingCorrector
	{
		uint64 operator() (uint64 emit, uint64 dispatch, uint64 step)
		{
			CAGE_ASSERT(step > 0);
			CAGE_ASSERT(dispatch > emit);
			uint64 r = dispatch + correction;
			if (r > emit + step)
				correction += sint64(step) / -500;
			else if (r < emit)
				correction += sint64(step) / 500;
			return max(emit, dispatch + correction);
		}

		InterpolationTimingCorrector() : correction(-70000)
		{}

		sint64 correction;
	};

	struct ClearOnScopeExit : private Immovable
	{
		template<class T>
		explicit ClearOnScopeExit(T *&ptr) : ptr((void*&)ptr)
		{}

		~ClearOnScopeExit()
		{
			ptr = nullptr;
		}

	private:
		void *&ptr;
	};
}

#include <optick.h>
