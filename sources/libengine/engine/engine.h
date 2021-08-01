#ifndef header_guard_engine_h_saf54g4ds4qaqq56q44olpoiu
#define header_guard_engine_h_saf54g4ds4qaqq56q44olpoiu

#include <cage-core/entities.h>
#include <cage-core/assetManager.h>
#include <cage-core/variableSmoothingBuffer.h>

#include <cage-engine/engine.h>

namespace cage
{
	void graphicsCreate(const EngineCreateConfig &config);
	void graphicsDestroy();
	void graphicsInitialize(); // opengl thread
	void graphicsFinalize(); // opengl thread
	void graphicsEmit(uint64 time); // control thread
	void graphicsPrepare(uint64 time, uint32 &drawCalls, uint32 &drawPrimitives); // prepare thread
	void graphicsDispatch(); // opengl thread
	void graphicsSwap(); // opengl thread

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

	extern EntityComponent *transformHistoryComponent;
}

#endif
