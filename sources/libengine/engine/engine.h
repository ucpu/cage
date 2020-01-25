#include <cage-core/variableSmoothingBuffer.h>

namespace cage
{
	void graphicsPrepareCreate(const EngineCreateConfig &config);
	void graphicsPrepareDestroy();
	void graphicsPrepareFinalize();
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
			corrections.add((sint64)emit - (sint64)dispatch);
			sint64 c = corrections.smooth();
			return max(emit, dispatch + c + step / 2);
		}

		VariableSmoothingBuffer<sint64, 100> corrections;
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
