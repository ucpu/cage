#ifndef header_guard_graphics_h_o46tgfs52d4ftg1zh8t5
#define header_guard_graphics_h_o46tgfs52d4ftg1zh8t5

#include <cage-engine/scene.h>

#include "../engine.h"

#include <vector>

namespace cage
{
	struct EmitBuffer : private Immovable
	{
		Holder<EntityManager> scene = newEntityManager();
		uint64 time = 0;
	};

	class Graphics : private Immovable
	{
	public:
		explicit Graphics(const EngineCreateConfig &config);
		~Graphics();
		void initialize(); // opengl thread
		void finalize(); // opengl thread
		void emit(uint64 time); // control thread
		void prepare(uint64 time); // prepare thread
		void dispatch(); // opengl thread
		void swap(); // opengl thread

		Holder<RenderQueue> renderQueue;
		Holder<ProvisionalGraphics> provisionalData;

		Holder<MemoryArena> emitBuffersMemory;
		Holder<SwapBufferGuard> emitBuffersGuard;
		EmitBuffer emitBuffers[3];
		InterpolationTimingCorrector itc;

		uint32 outputDrawCalls = 0;
		uint32 outputDrawPrimitives = 0;
		uint32 frameIndex = 0;
	};

	extern Graphics *graphics;
}

#endif