#ifndef header_guard_graphics_h_o46tgfs52d4ftg1zh8t5
#define header_guard_graphics_h_o46tgfs52d4ftg1zh8t5

#include "../engine.h"

#include <vector>

namespace cage
{
	struct EmitTransform
	{
		TransformComponent current, history;
		uintPtr entityId = 0;
	};

	struct EmitRender : public EmitTransform
	{
		RenderComponent render;
		Holder<SkeletalAnimationComponent> skeletalAnimation;
		Holder<TextureAnimationComponent> textureAnimation;
	};

	struct EmitText : public EmitTransform
	{
		TextComponent text;
	};

	struct EmitLight : public EmitTransform
	{
		LightComponent light;
		Holder<ShadowmapComponent> shadowmap;
	};

	struct EmitCamera : public EmitTransform
	{
		CameraComponent camera;
	};

	struct EmitBuffer : private Immovable
	{
		std::vector<EmitRender> renders;
		std::vector<EmitText> texts;
		std::vector<EmitLight> lights;
		std::vector<EmitCamera> cameras;
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
