#include <cage-core/pointerRangeHolder.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-core/hashString.h>
#include <cage-core/config.h>
#include <cage-core/color.h>

#include <cage-engine/provisionalGraphics.h>
#include <cage-engine/provisionalHandles.h>
#include <cage-engine/screenSpaceEffects.h>
#include <cage-engine/shaderConventions.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/window.h>
#include <cage-engine/opengl.h> // all the constants

#include <cage-engine/shaderProgram.h>
#include <cage-engine/renderObject.h>
#include <cage-engine/texture.h>
#include <cage-engine/model.h>
#include <cage-engine/font.h>

#include "graphics.h"

namespace cage
{
	// implemented in gui
	string loadInternationalizedText(AssetManager *assets, uint32 asset, uint32 text, string params);

	namespace
	{
		ConfigBool confRenderMissingModels("cage/graphics/renderMissingModels", false);
		ConfigBool confRenderSkeletonBones("cage/graphics/renderSkeletonBones", false);
		ConfigBool confNoAmbientOcclusion("cage/graphics/disableAmbientOcclusion", false);
		ConfigBool confNoBloom("cage/graphics/disableBloom", false);
		ConfigBool confNoMotionBlur("cage/graphics/disableMotionBlur", false);

		struct Mat3x4
		{
			vec4 data[3];

			Mat3x4() : Mat3x4(mat4())
			{}

			explicit Mat3x4(const mat3 &in)
			{
				for (uint32 a = 0; a < 3; a++)
					for (uint32 b = 0; b < 3; b++)
						data[b][a] = in[a * 3 + b];
			}

			explicit Mat3x4(const mat4 &in)
			{
				CAGE_ASSERT(in[3] == 0 && in[7] == 0 && in[11] == 0 && in[15] == 1);
				for (uint32 a = 0; a < 4; a++)
					for (uint32 b = 0; b < 3; b++)
						data[b][a] = in[a * 4 + b];
			}
		};

		struct UniModel
		{
			mat4 mvpMat;
			mat4 mvpPrevMat;
			Mat3x4 normalMat; // [2][3] is 1 if lighting is enabled and 0 otherwise
			Mat3x4 mMat;
			vec4 color; // color rgb is linear, and NOT alpha-premultiplied
			vec4 aniTexFrames;
		};

		struct UniLight
		{
			mat4 shadowMat;
			mat4 mvpMat;
			vec4 color; // + spotAngle
			vec4 position;
			vec4 direction; // + normalOffsetScale
			vec4 attenuation; // + spotExponent
		};

		struct UniViewport
		{
			mat4 vpInv;
			vec4 eyePos;
			vec4 eyeDir;
			vec4 viewport;
			vec4 ambientLight;
			vec4 ambientDirectionalLight;
		};

		struct PrepTransform
		{
			mat4 model;
			mat4 modelPrev;
		};

		struct PrepSkeleton : private Noncopyable
		{
			Holder<const SkeletonRig> rig;
			Holder<const SkeletalAnimation> animation;
			SkeletalAnimationComponent params;
			std::vector<Mat3x4> armature;
		};

		struct PrepTexture : private Immovable
		{
			TextureAnimationComponent params;
		};

		struct PrepRender : public PrepTransform
		{
			const EmitRender *emit = nullptr;
			RenderComponent render;
			Holder<const RenderObject> object;
			Holder<PrepSkeleton> skeletalAnimation;
			Holder<PrepTexture> textureAnimation;

			PrepRender(const EmitRender *e) : emit(e)
			{}
		};

		struct PrepText : public PrepTransform
		{
			const EmitText *emit = nullptr;
			TextComponent text;
			Holder<Font> font;
			std::vector<uint32> glyphs;
			FontFormat format;
			vec3 color;

			PrepText(const EmitText *e) : emit(e)
			{}
		};

		struct PrepLight : public PrepTransform
		{
			const EmitLight *emit = nullptr;

			PrepLight(const EmitLight *e) : emit(e)
			{}
		};

		struct PrepCamera : public PrepTransform
		{
			const EmitCamera *emit = nullptr;

			PrepCamera(const EmitCamera *e) : emit(e)
			{}
		};

		struct Preparator
		{
			const EmitBuffer &emit;

			uint64 prepareTime = 0;
			real interFactor;
			ivec2 windowResolution;

			Holder<PointerRange<mat4>> tmpArmature = []() { PointerRangeHolder<mat4> prh; prh.resize(CAGE_SHADER_MAX_BONES); return prh; }();

			Holder<Model> modelSquare, modelSphere, modelCone;
			Holder<ShaderProgram> shaderAmbient, shaderBlit, shaderDepth, shaderGBuffer, shaderLighting, shaderTranslucent;
			Holder<ShaderProgram> shaderVisualizeColor, shaderFont;

			std::vector<PrepRender> renders;
			std::vector<PrepText> texts;
			std::vector<PrepLight> lights;
			std::vector<PrepCamera> cameras;

			Preparator(const EmitBuffer &emit, uint64 time) : emit(emit)
			{
				prepareTime = graphics->itc(emit.time, time, controlThread().updatePeriod());
				interFactor = saturate(real(prepareTime - emit.time) / controlThread().updatePeriod());
			}

			bool loadGeneralAssets()
			{
				AssetManager *ass = engineAssets();
				if (!ass->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/cage.pack")) || !ass->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/shader/engine/engine.pack")))
					return false;

				modelSquare = ass->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
				modelSphere = ass->get<AssetSchemeIndexModel, Model>(HashString("cage/model/sphere.obj"));
				modelCone = ass->get<AssetSchemeIndexModel, Model>(HashString("cage/model/cone.obj"));
				shaderAmbient = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/ambient.glsl"));
				shaderBlit = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/blit.glsl"));
				shaderDepth = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/depth.glsl"));
				shaderGBuffer = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/gBuffer.glsl"));
				shaderLighting = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/lighting.glsl"));
				shaderTranslucent = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/translucent.glsl"));
				shaderVisualizeColor = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/visualize/color.glsl"));
				shaderFont = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/font.glsl"));
				CAGE_ASSERT(shaderBlit);
				return true;
			}

			template<class E, class P>
			void copyEmitToPrep(const std::vector<E> &es, std::vector<P> &ps)
			{
				ps.reserve(es.size());
				for (const auto &it : es)
					ps.emplace_back(&it);
				for (auto &it : ps)
				{
					it.model = mat4(interpolate(it.emit->history, it.emit->current, interFactor));
					it.modelPrev = mat4(interpolate(it.emit->history, it.emit->current, interFactor - 0.2));
				}
			}

			void copyEmitToPrep()
			{
				copyEmitToPrep(emit.renders, renders);
				copyEmitToPrep(emit.texts, texts);
				copyEmitToPrep(emit.lights, lights);
				copyEmitToPrep(emit.cameras, cameras);
			}

			[[deprecated]] // skeleton rig should be referenced from animations instead of models
			uint32 findSkeletonRigName(PrepRender &pr)
			{
				if (pr.object)
				{
					for (uint32 l = 0; l < pr.object->lodsCount(); l++)
					{
						for (uint32 i : pr.object->models(l))
						{
							Holder<Model> mo = engineAssets()->tryGet<AssetSchemeIndexModel, Model>(i);
							if (mo && mo->skeletonName)
								return mo->skeletonName;
						}
					}
					return 0;
				}
				else
				{
					Holder<Model> mo = engineAssets()->tryGet<AssetSchemeIndexModel, Model>(pr.render.object);
					if (mo && mo->skeletonName)
						return mo->skeletonName;
					return 0;
				}
			}

			void updateCommonValues(PrepRender &pr)
			{
				pr.render = pr.emit->render;
				if (!pr.render.object)
					return;

				pr.object = engineAssets()->tryGet<AssetSchemeIndexRenderObject, RenderObject>(pr.render.object);
				if (!pr.object && !engineAssets()->tryGet<AssetSchemeIndexModel, Model>(pr.render.object))
				{
					if (!confRenderMissingModels)
					{
						pr.render.object = 0; // disable rendering further in the pipeline
						return;
					}
					pr.render.object = HashString("cage/model/fake.obj");
				}

				Holder<PrepTexture> &pt = pr.textureAnimation;
				if (pr.emit->textureAnimation)
				{
					pt = systemMemory().createHolder<PrepTexture>();
					pt->params = *pr.emit->textureAnimation;
				}

				Holder<PrepSkeleton> &ps = pr.skeletalAnimation;
				if (pr.emit->skeletalAnimation)
				{
					ps = systemMemory().createHolder<PrepSkeleton>();
					ps->params = *pr.emit->skeletalAnimation;
				}

				if (pr.object)
				{
					if (!pr.render.color.valid())
						pr.render.color = pr.object->color;
					if (!pr.render.intensity.valid())
						pr.render.intensity = pr.object->intensity;
					if (!pr.render.opacity.valid())
						pr.render.opacity = pr.object->opacity;

					if (!pt && (pr.object->texAnimSpeed.valid() || pr.object->texAnimOffset.valid()))
						pt = systemMemory().createHolder<PrepTexture>();
					if (pt)
					{
						if (!pt->params.speed.valid())
							pt->params.speed = pr.object->texAnimSpeed;
						if (!pt->params.offset.valid())
							pt->params.offset = pr.object->texAnimOffset;
					}

					if (!ps && pr.object->skelAnimName)
						ps = systemMemory().createHolder<PrepSkeleton>();
					if (ps)
					{
						if (!ps->params.name)
							ps->params.name = pr.object->skelAnimName;
						if (!ps->params.speed.valid())
							ps->params.speed = pr.object->skelAnimSpeed;
						if (!ps->params.offset.valid())
							ps->params.offset = pr.object->skelAnimOffset;
					}
				}

				if (!pr.render.color.valid())
					pr.render.color = vec3(0);
				if (!pr.render.intensity.valid())
					pr.render.intensity = 1;
				if (!pr.render.opacity.valid())
					pr.render.opacity = 1;

				if (pt)
				{
					if (!pt->params.speed.valid())
						pt->params.speed = 1;
					if (!pt->params.offset.valid())
						pt->params.offset = 0;
				}

				if (ps)
				{
					if (ps->params.name)
					{
						if (!ps->params.speed.valid())
							ps->params.speed = 1;
						if (!ps->params.offset.valid())
							ps->params.offset = 0;
					}
					else
						ps.clear();
				}

				if (ps)
				{
					ps->animation = engineAssets()->tryGet<AssetSchemeIndexSkeletalAnimation, SkeletalAnimation>(ps->params.name);
					ps->rig = engineAssets()->tryGet<AssetSchemeIndexSkeletonRig, SkeletonRig>(findSkeletonRigName(pr)); // todo use rig name from the animation
					if (!ps->animation || !ps->rig)
						ps.clear();
				}

				if (ps)
				{
					const uint32 bonesCount = ps->rig->bonesCount();
					CAGE_ASSERT(ps->animation->bonesCount() == bonesCount);
					const real c = detail::evalCoefficientForSkeletalAnimation(+ps->animation, prepareTime, ps->params.startTime, ps->params.speed, ps->params.offset);
					animateSkin(+ps->rig, +ps->animation, c, tmpArmature);
					ps->armature.reserve(bonesCount);
					for (uint32 i = 0; i < bonesCount; i++)
						ps->armature.emplace_back(tmpArmature[i]);
				}
			}

			void updateCommonValues(PrepText &pt)
			{
				pt.text = pt.emit->text;

				if (!pt.text.font)
					pt.text.font = HashString("cage/font/ubuntu/Ubuntu-R.ttf");
				pt.font = engineAssets()->tryGet<AssetSchemeIndexFont, Font>(pt.text.font);
				if (!pt.font)
					return;

				// todo default color, intensity & opacity

				const string str = loadInternationalizedText(engineAssets(), pt.text.assetName, pt.text.textName, pt.text.value);
				const uint32 count = pt.font->glyphsCount(str);
				if (count == 0)
				{
					pt.font.clear();
					return;
				}
				pt.glyphs.resize(count);
				pt.font->transcript(str, pt.glyphs);
				pt.color = colorGammaToLinear(pt.text.color) * pt.text.intensity;
				pt.format.size = 1;
				const vec2 size = pt.font->size(pt.glyphs, pt.format);
				pt.format.wrapWidth = size[0];
				const mat4 tr = mat4(vec3(size * vec2(-0.5, 0.5), 0));
				pt.model *= tr;
				pt.modelPrev *= tr;
			}

			void updateCommonValues()
			{
				for (auto &it : renders)
					updateCommonValues(it);
				for (auto &it : texts)
					updateCommonValues(it);
			}

			void prepare()
			{
				windowResolution = engineWindow()->resolution();
				if (windowResolution[0] <= 0 || windowResolution[1] <= 0)
					return;
				if (!loadGeneralAssets())
					return;
				copyEmitToPrep();
				updateCommonValues();

				// todo
			}
		};
	}

	void Graphics::prepare(uint64 time)
	{
		renderQueue->resetQueue();

		if (auto lock = emitBuffersGuard->read())
		{
			Preparator prep(emitBuffers[lock.index()], time);
			prep.prepare();
		}
	}
}
