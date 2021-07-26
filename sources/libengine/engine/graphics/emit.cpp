#include <cage-core/entitiesVisitor.h>
#include <cage-core/swapBufferGuard.h>

#include "graphics.h"

namespace cage
{
	namespace
	{
		void copyTransform(EmitTransform &ee, Entity *e, const TransformComponent &tr)
		{
			ee.current = tr;
			if (e->has(transformHistoryComponent))
				ee.history = e->value<TransformComponent>(transformHistoryComponent);
			else
				ee.history = tr;
			ee.entityId = (uintPtr)e;
		}

		void emittor(EmitBuffer &eb)
		{
			eb.renders.clear();
			eb.texts.clear();
			eb.lights.clear();
			eb.cameras.clear();

			auto &mem = graphics->emitBuffersMemory;
			EntityComponent *skeletalAnimationComponent = engineEntities()->component<SkeletalAnimationComponent>();
			EntityComponent *textureAnimationComponent = engineEntities()->component<TextureAnimationComponent>();
			EntityComponent *shadowmapComponent = engineEntities()->component<ShadowmapComponent>();

			entitiesVisitor(engineEntities(), [&](Entity *e, const TransformComponent &tr, const RenderComponent &re) {
				EmitRender ee;
				copyTransform(ee, e, tr);
				ee.render = re;
				if (e->has(skeletalAnimationComponent))
					ee.skeletalAnimation = mem->createHolder<SkeletalAnimationComponent>(e->value<SkeletalAnimationComponent>(skeletalAnimationComponent));
				if (e->has(textureAnimationComponent))
					ee.textureAnimation = mem->createHolder<TextureAnimationComponent>(e->value<TextureAnimationComponent>(textureAnimationComponent));
				eb.renders.push_back(std::move(ee));
			});

			entitiesVisitor(engineEntities(), [&](Entity *e, const TransformComponent &tr, const TextComponent &te) {
				EmitText ee;
				copyTransform(ee, e, tr);
				ee.text = te;
				eb.texts.push_back(std::move(ee));
			});

			entitiesVisitor(engineEntities(), [&](Entity *e, const TransformComponent &tr, const LightComponent &li) {
				EmitLight ee;
				copyTransform(ee, e, tr);
				ee.light = li;
				//if (e->has(shadowmapComponent))
				//	ee.shadowmap = mem->createHolder<ShadowmapComponent>(e->value<ShadowmapComponent>(shadowmapComponent));
				eb.lights.push_back(std::move(ee));
			});

			entitiesVisitor(engineEntities(), [&](Entity *e, const TransformComponent &tr, const CameraComponent &cam) {
				EmitCamera ee;
				copyTransform(ee, e, tr);
				ee.camera = cam;
				eb.cameras.push_back(std::move(ee));
			});
		}
	}

	void Graphics::emit(uint64 time)
	{
		if (auto lock = emitBuffersGuard->write())
		{
			emittor(emitBuffers[lock.index()]);
			emitBuffers[lock.index()].time = time;
		}
	}
}
