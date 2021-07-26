#include <cage-core/pointerRangeHolder.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-core/hashString.h>
#include <cage-core/geometry.h>
#include <cage-core/camera.h>
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

#include <robin_hood.h>

namespace cage
{
	// implemented in gui
	string loadInternationalizedText(AssetManager *assets, uint32 asset, uint32 text, string params);

	namespace
	{
		ConfigBool confRenderMissingModels("cage/graphics/renderMissingModels", false);
		ConfigBool confRenderSkeletonBones("cage/graphics/renderSkeletonBones", false);
		ConfigBool confNoAmbientOcclusion("cage/graphics/disableAmbientOcclusion", false);
		ConfigBool confNoMotionBlur("cage/graphics/disableMotionBlur", false);
		ConfigBool confNoBloom("cage/graphics/disableBloom", false);

		template<class T>
		PointerRange<T> subRange(PointerRange<T> base, uint32 off, uint32 num)
		{
			return { base.begin() + off, base.begin() + off + num };
		}

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
			mat4 mvpMatPrev;
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
			vec4 viewport; // x, y, w, h
			vec4 ambientLight;
			vec4 ambientDirectionalLight;
		};

		struct PrepTransform
		{
			mat4 model;
			mat4 modelPrev;
		};

		struct PrepSkeleton : private Immovable
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
			CameraComponent camera;
			Holder<Texture> target;

			PrepCamera(const EmitCamera *e) : emit(e)
			{}
		};

		struct RendersInstances : private Noncopyable
		{
			std::vector<UniModel> data;
			std::vector<PointerRange<const Mat3x4>> armatures;
			Holder<Model> model;
			Holder<Texture> textures[MaxTexturesCountPerMaterial];
			uint32 shaderRoutines[CAGE_SHADER_MAX_ROUTINES] = {};
		};

		struct RendersCollection : private Noncopyable
		{
			struct Key
			{
				Model *model = nullptr;
				bool skeletalAnimation = false;
			};
			struct Hasher
			{
				auto operator () (const Key &k) const
				{
					return std::hash<Model *>()(k.model) + k.skeletalAnimation;
				}
				bool operator () (const Key &a, const Key &b) const
				{
					return a.model == b.model && a.skeletalAnimation == b.skeletalAnimation;
				}
			};
			robin_hood::unordered_map<Key, RendersInstances, Hasher, Hasher> data;
		};

		struct TextsInstances : private Noncopyable
		{
			std::vector<PrepText *> data;
		};

		struct TextsCollection : private Noncopyable
		{
			robin_hood::unordered_map<Font *, TextsInstances> data;
		};

		struct ShadowedLight : private Noncopyable
		{
			PrepLight *light = nullptr;
			UniLight data;
			struct ShadowmapPass *shadowmap = nullptr;
		};

		struct LightsCollection : private Noncopyable
		{
			std::vector<ShadowedLight> shadowed;
			std::vector<UniLight> directional, spot, point;
		};

		struct TranslucentsInstances : private Noncopyable
		{
			RendersInstances renders;
			LightsCollection lights;
			Aabb box; // world-space box for the renders - used for culling lights
		};

		struct TranslucentsCollection : private Noncopyable
		{
			std::vector<TranslucentsInstances> data;
		};

		struct CommonRenderPass : private Noncopyable
		{
			const PrepCamera *camera = nullptr;
			Frustum frustum; // world-space frustum of the camera/shadow
			mat4 view;
			mat4 viewPrev;
			mat4 proj;
			mat4 viewProj;
			mat4 viewProjPrev;
			ivec2 resolution;

			struct LodSelection
			{
				vec3 center; // center of camera (NOT light)
				real screenSize = 0; // vertical size of screen in pixels, one meter in front of the camera
				bool orthographic = false;
			} lodSelection;
		};

		struct ShadowmapPass : public CommonRenderPass
		{
			const PrepLight *light = nullptr;
			RendersCollection shadowers;
			mat4 shadowMat;
			TextureHandle depthTexture;
		};

		struct CameraPass : public CommonRenderPass
		{
			UniViewport viewport;
			std::vector<ShadowmapPass> shadowmapPasses;
			RendersCollection opaque;
			LightsCollection lights;
			TranslucentsCollection translucents;
			TextsCollection texts;

			FrameBufferHandle renderTarget;
			FrameBufferHandle gBufferTarget;
			TextureHandle albedoTexture;
			TextureHandle specialTexture;
			TextureHandle normalTexture;
			TextureHandle depthTexture;
			TextureHandle colorTexture;
			TextureHandle intermediateTexture;
			TextureHandle velocityTexture;
			TextureHandle ambientOcclusionTexture;
			TextureHandle texSource;
			TextureHandle texTarget;
		};

		struct Preparator
		{
			const EmitBuffer &emit;
			const Holder<RenderQueue> &renderQueue = graphics->renderQueue;
			const Holder<ProvisionalGraphics> &provisionalData = graphics->provisionalData;
			const uint32 frameIndex = graphics->frameIndex;

			uint64 prepareTime = 0;
			real interFactor;
			ivec2 windowResolution;

			bool cnfRenderMissingModels = confRenderMissingModels;
			bool cnfRenderSkeletonBones = confRenderSkeletonBones;
			bool cnfNoAmbientOcclusion = confNoAmbientOcclusion;
			bool cnfNoMotionBlur = confNoMotionBlur;
			bool cnfNoBloom = confNoBloom;

			Holder<PointerRange<mat4>> tmpArmature = []() { PointerRangeHolder<mat4> prh; prh.resize(CAGE_SHADER_MAX_BONES); return prh; }();

			Holder<Model> modelSquare, modelSphere, modelCone, modelBone;
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
				windowResolution = engineWindow()->resolution();
			}

			bool loadGeneralAssets()
			{
				AssetManager *ass = engineAssets();
				if (!ass->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/cage.pack")) || !ass->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/shader/engine/engine.pack")))
					return false;

				modelSquare = ass->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
				modelSphere = ass->get<AssetSchemeIndexModel, Model>(HashString("cage/model/sphere.obj"));
				modelCone = ass->get<AssetSchemeIndexModel, Model>(HashString("cage/model/cone.obj"));
				modelBone = engineAssets()->get<AssetSchemeIndexModel, Model>(HashString("cage/model/bone.obj"));
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
					if (!cnfRenderMissingModels)
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
					CAGE_ASSERT(bonesCount > 0);
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

			void updateCommonValues(PrepCamera &pc)
			{
				pc.camera = pc.emit->camera;

				if (cnfNoAmbientOcclusion)
					pc.camera.effects &= ~CameraEffectsFlags::AmbientOcclusion;
				if (cnfNoMotionBlur)
					pc.camera.effects &= ~CameraEffectsFlags::MotionBlur;
				if (cnfNoBloom)
					pc.camera.effects &= ~CameraEffectsFlags::Bloom;
			}

			void updateCommonValues()
			{
				for (auto &it : renders)
					updateCommonValues(it);
				for (auto &it : texts)
					updateCommonValues(it);
				for (auto &it : cameras)
					updateCommonValues(it);
			}

			void addModels(const CommonRenderPass &pass, const PrepRender &pr, Holder<Model> mo, RendersInstances &instances)
			{
				UniModel sm;
				sm.color = vec4(colorGammaToLinear(pr.render.color) * pr.render.intensity, pr.render.opacity);
				sm.mMat = Mat3x4(pr.model);
				sm.mvpMat = pass.viewProj * pr.model;
				sm.mvpMatPrev = any(mo->flags & ModelRenderFlags::VelocityWrite) ? pass.viewProjPrev * pr.modelPrev : sm.mvpMat;
				sm.normalMat = Mat3x4(inverse(mat3(pr.model)));
				sm.normalMat.data[2][3] = any(mo->flags & ModelRenderFlags::Lighting) ? 1 : 0; // is lighting enabled
				if (pr.textureAnimation)
				{
					Holder<Texture> t = engineAssets()->tryGet<AssetSchemeIndexTexture, Texture>(mo->textureName(CAGE_SHADER_TEXTURE_ALBEDO));
					if (t)
					{
						const auto &p = pr.textureAnimation->params;
						sm.aniTexFrames = detail::evalSamplesForTextureAnimation(+t, prepareTime, p.startTime, p.speed, p.offset);
					}
				}
				instances.data.push_back(sm);

				if (pr.skeletalAnimation)
				{
					instances.armatures.push_back(pr.skeletalAnimation->armature);
					CAGE_ASSERT(instances.armatures.size() == instances.data.size());
				}

				if (!instances.model)
				{
					instances.model = mo.share();

					for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
					{
						const uint32 n = instances.model->textureName(i);
						if (!n)
							continue;
						instances.textures[i] = engineAssets()->tryGet<AssetSchemeIndexTexture, Texture>(n);
					}

					instances.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_SKELETON] = pr.skeletalAnimation ? CAGE_SHADER_ROUTINEPROC_SKELETONANIMATION : CAGE_SHADER_ROUTINEPROC_SKELETONNOTHING;

					if (instances.textures[CAGE_SHADER_TEXTURE_ALBEDO])
					{
						switch (instances.textures[CAGE_SHADER_TEXTURE_ALBEDO]->target())
						{
						case GL_TEXTURE_2D_ARRAY:
							instances.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPALBEDO] = CAGE_SHADER_ROUTINEPROC_MAPALBEDOARRAY;
							break;
						case GL_TEXTURE_CUBE_MAP:
							instances.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPALBEDO] = CAGE_SHADER_ROUTINEPROC_MAPALBEDOCUBE;
							break;
						default:
							instances.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPALBEDO] = CAGE_SHADER_ROUTINEPROC_MAPALBEDO2D;
							break;
						}
					}
					else
						instances.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPALBEDO] = CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING;

					if (instances.textures[CAGE_SHADER_TEXTURE_SPECIAL])
					{
						switch (instances.textures[CAGE_SHADER_TEXTURE_SPECIAL]->target())
						{
						case GL_TEXTURE_2D_ARRAY:
							instances.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL] = CAGE_SHADER_ROUTINEPROC_MAPSPECIALARRAY;
							break;
						case GL_TEXTURE_CUBE_MAP:
							instances.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL] = CAGE_SHADER_ROUTINEPROC_MAPSPECIALCUBE;
							break;
						default:
							instances.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL] = CAGE_SHADER_ROUTINEPROC_MAPSPECIAL2D;
							break;
						}
					}
					else
						instances.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL] = CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING;

					if (instances.textures[CAGE_SHADER_TEXTURE_NORMAL])
					{
						switch (instances.textures[CAGE_SHADER_TEXTURE_NORMAL]->target())
						{
						case GL_TEXTURE_2D_ARRAY:
							instances.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPNORMAL] = CAGE_SHADER_ROUTINEPROC_MAPNORMALARRAY;
							break;
						case GL_TEXTURE_CUBE_MAP:
							instances.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPNORMAL] = CAGE_SHADER_ROUTINEPROC_MAPNORMALCUBE;
							break;
						default:
							instances.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPNORMAL] = CAGE_SHADER_ROUTINEPROC_MAPNORMAL2D;
							break;
						}
					}
					else
						instances.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPNORMAL] = CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING;
				}

				CAGE_ASSERT(+instances.model == +mo);
			}

			template<bool ShadowCastersOnly>
			void addModels(const CommonRenderPass &pass, const PrepRender &pr, Holder<Model> mo, RendersCollection &opaque, TranslucentsCollection &translucents)
			{
				if constexpr (ShadowCastersOnly)
				{
					if (none(mo->flags & ModelRenderFlags::ShadowCast))
						return;
				}

				const Aabb box = mo->boundingBox() * pr.model;
				if (!intersects(box, pass.frustum))
					return;

				if constexpr (!ShadowCastersOnly)
				{
					if (any(mo->flags & ModelRenderFlags::Translucent) || pr.render.opacity < 1)
					{
						translucents.data.emplace_back();
						translucents.data.back().box = box;
						addModels(pass, pr, std::move(mo), translucents.data.back().renders);
						return;
					}
				}

				addModels(pass, pr, mo.share(), opaque.data[{ +mo, !!pr.skeletalAnimation }]);
			}

			template<bool ShadowCastersOnly>
			void addModelsSkeleton(const CommonRenderPass &pass, const PrepRender &pr, RendersCollection &opaque, TranslucentsCollection &translucents)
			{
				const auto &s = pr.skeletalAnimation->rig;
				const auto &a = pr.skeletalAnimation->animation;
				const auto &p = pr.skeletalAnimation->params;
				const uint32 bonesCount = s->bonesCount();
				const real c = detail::evalCoefficientForSkeletalAnimation(+a, prepareTime, p.startTime, p.speed, p.offset);
				animateSkeleton(+s, +a, c, tmpArmature);
				for (uint32 i = 0; i < bonesCount; i++)
				{
					PrepRender r(pr.emit);
					r.model = pr.model * tmpArmature[i];
					r.modelPrev = pr.modelPrev * tmpArmature[i];
					r.render = pr.render;
					r.render.color = colorHsvToRgb(vec3(real(i) / real(bonesCount), 1, 1));
					r.render.object = 0;
					addModels<ShadowCastersOnly>(pass, r, modelBone.share(), opaque, translucents);
				}
			}

			template<bool ShadowCastersOnly>
			void addModels(const CommonRenderPass &pass, const PrepRender &pr, RendersCollection &opaque, TranslucentsCollection &translucents)
			{
				CAGE_ASSERT(pass.lodSelection.screenSize > 0);
				if ((pr.render.sceneMask & pass.camera->camera.sceneMask) == 0 || pr.render.object == 0)
					return;

				if (cnfRenderSkeletonBones && pr.skeletalAnimation)
				{
					addModelsSkeleton<ShadowCastersOnly>(pass, pr, opaque, translucents);
					return;
				}

				if (pr.object)
				{
					CAGE_ASSERT(pr.object->lodsCount() > 0);
					uint32 lod = 0;
					if (pr.object->lodsCount() > 1)
					{
						real d = 1;
						if (!pass.lodSelection.orthographic)
						{
							const vec4 ep4 = pr.model * vec4(0, 0, 0, 1);
							CAGE_ASSERT(abs(ep4[3] - 1) < 1e-4);
							d = distance(vec3(ep4), pass.lodSelection.center);
						}
						const real f = pass.lodSelection.screenSize * pr.object->worldSize / (d * pr.object->pixelsSize);
						lod = pr.object->lodSelect(f);
					}
					for (uint32 msh : pr.object->models(lod))
					{
						Holder<Model> mo = engineAssets()->get<AssetSchemeIndexModel, Model>(msh);
						if (mo)
							addModels<ShadowCastersOnly>(pass, pr, std::move(mo), opaque, translucents);
					}
				}
				else
				{
					Holder<Model> mo = engineAssets()->tryGet<AssetSchemeIndexModel, Model>(pr.render.object);
					if (mo)
						addModels<ShadowCastersOnly>(pass, pr, std::move(mo), opaque, translucents);
				}
			}

			void addModels(CameraPass &pass, const PrepRender &pr)
			{
				addModels<false>(pass, pr, pass.opaque, pass.translucents);
			}

			void addModels(ShadowmapPass &pass, const PrepRender &pr)
			{
				TranslucentsCollection trans;
				addModels<true>(pass, pr, pass.shadowers, trans);
				CAGE_ASSERT(trans.data.empty());
			}

			static void initializeLodSelectionAndFrustum(CommonRenderPass &pass)
			{
				const auto &c = pass.camera->camera;
				switch (c.cameraType)
				{
				case CameraTypeEnum::Orthographic:
				{
					pass.lodSelection.screenSize = c.camera.orthographicSize[1] * pass.resolution[1];
					pass.lodSelection.orthographic = true;
				} break;
				case CameraTypeEnum::Perspective:
					pass.lodSelection.screenSize = tan(c.camera.perspectiveFov * 0.5) * 2 * pass.resolution[1];
					break;
				default:
					CAGE_THROW_ERROR(Exception, "invalid camera type");
				}
				pass.lodSelection.center = vec3(pass.camera->model * vec4(0, 0, 0, 1));
				pass.frustum = Frustum(pass.viewProj);
			}

			void generateShadowmapPass(ShadowmapPass &pass)
			{
				const LightComponent &lc = pass.light->emit->light;
				const ShadowmapComponent &sc = *pass.light->emit->shadowmap;

				pass.resolution = ivec2(sc.resolution);
				pass.view = pass.viewPrev = inverse(pass.light->model);
				switch (lc.lightType)
				{
				case LightTypeEnum::Directional:
					pass.proj = orthographicProjection(-sc.worldSize[0], sc.worldSize[0], -sc.worldSize[1], sc.worldSize[1], -sc.worldSize[2], sc.worldSize[2]);
					break;
				case LightTypeEnum::Spot:
					pass.proj = perspectiveProjection(lc.spotAngle, 1, sc.worldSize[0], sc.worldSize[1]);
					break;
				case LightTypeEnum::Point:
					pass.proj = perspectiveProjection(degs(90), 1, sc.worldSize[0], sc.worldSize[1]);
					break;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid light type");
				}
				pass.viewProj = pass.viewProjPrev = pass.proj * pass.view;

				constexpr mat4 bias = mat4(
					0.5, 0.0, 0.0, 0.0,
					0.0, 0.5, 0.0, 0.0,
					0.0, 0.0, 0.5, 0.0,
					0.5, 0.5, 0.5, 1.0);
				pass.shadowMat = bias * pass.viewProj;

				initializeLodSelectionAndFrustum(pass);

				for (const PrepRender &pr : renders)
					addModels(pass, pr);

				// todo prepare render target
			}

			void generateCameraPass(CameraPass &pass)
			{
				const CameraComponent &cc = pass.camera->camera;

				pass.resolution = pass.camera->target ? pass.camera->target->resolution() : windowResolution;
				pass.view = inverse(pass.camera->model);
				pass.viewPrev = inverse(pass.camera->modelPrev);
				switch (cc.cameraType)
				{
				case CameraTypeEnum::Orthographic:
				{
					const vec2 &os = cc.camera.orthographicSize;
					pass.proj = orthographicProjection(-os[0], os[0], -os[1], os[1], cc.near, cc.far);
				} break;
				case CameraTypeEnum::Perspective:
					pass.proj = perspectiveProjection(cc.camera.perspectiveFov, real(pass.resolution[0]) / real(pass.resolution[1]), cc.near, cc.far);
					break;
				default:
					CAGE_THROW_ERROR(Exception, "invalid camera type");
				}
				pass.viewProj = pass.proj * pass.view;
				pass.viewProjPrev = pass.proj * pass.viewPrev;

				pass.viewport.vpInv = inverse(pass.viewProj);
				pass.viewport.eyePos = pass.camera->model * vec4(0, 0, 0, 1);
				pass.viewport.eyeDir = pass.camera->model * vec4(0, 0, -1, 0);
				pass.viewport.ambientLight = vec4(colorGammaToLinear(cc.ambientColor) * cc.ambientIntensity, 0);
				pass.viewport.ambientDirectionalLight = vec4(colorGammaToLinear(cc.ambientDirectionalColor) * cc.ambientDirectionalIntensity, 0);
				pass.viewport.viewport = vec4(vec2(), vec2(pass.resolution));

				initializeLodSelectionAndFrustum(pass);

				for (const PrepRender &pr : renders)
					addModels(pass, pr);

				// todo add renderable texts
				// todo add lights

				// todo generate shadowmaps passes
			}

			TextureHandle prepareGTexture(const char *name, uint32 internalFormat, const ivec2 &resolution, bool enabled = true)
			{
				if (!enabled)
					return {};
				TextureHandle t = provisionalData->texture(name);
				renderQueue->bind(t, 0);
				renderQueue->filters(GL_LINEAR, GL_LINEAR, 0);
				renderQueue->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
				renderQueue->image2d(resolution, internalFormat);
				return t;
			}

			void renderInstancesSkeletons(const RendersInstances &instances)
			{
				CAGE_ASSERT(instances.armatures.size() == instances.data.size());
				const uint32 bones = instances.model->skeletonBones;
				const uint32 drawLimit = min((uint32)CAGE_SHADER_MAX_INSTANCES, CAGE_SHADER_MAX_BONES / bones);
				renderQueue->uniform(CAGE_SHADER_UNI_BONESPERINSTANCE, bones);
				Mat3x4 tmpArmature[CAGE_SHADER_MAX_BONES];
				uint32 off = 0;
				while (off < instances.data.size())
				{
					const uint32 n = min(numeric_cast<uint32>(instances.data.size() - off), drawLimit);
					for (uint32 i = 0; i < n; i++)
					{
						CAGE_ASSERT(instances.armatures[off + i].size() == bones);
						detail::memcpy(tmpArmature + i * bones, instances.armatures[off + i].begin(), bones * sizeof(Mat3x4));
					}
					renderQueue->universalUniformArray(subRange<const Mat3x4>(tmpArmature, 0, n * bones), CAGE_SHADER_UNIBLOCK_ARMATURES);
					renderQueue->universalUniformArray(subRange<const UniModel>(instances.data, off, n), CAGE_SHADER_UNIBLOCK_MESHES);
					renderQueue->draw(n);
					off += n;
				}
				CAGE_ASSERT(off == instances.data.size());
			}

			void renderInstances(const RendersInstances &instances)
			{
				renderQueue->bind(instances.model);
				renderQueue->uniform(CAGE_SHADER_UNI_ROUTINES, instances.shaderRoutines);
				for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
				{
					const Holder<Texture> &t = instances.textures[i];
					if (!t)
						continue;
					switch (t->target())
					{
					case GL_TEXTURE_2D_ARRAY:
						renderQueue->bind(t, CAGE_SHADER_TEXTURE_ALBEDO_ARRAY + i);
						break;
					case GL_TEXTURE_CUBE_MAP:
						renderQueue->bind(t, CAGE_SHADER_TEXTURE_ALBEDO_CUBE + i);
						break;
					default:
						renderQueue->bind(t, CAGE_SHADER_TEXTURE_ALBEDO + i);
						break;
					}
				}
				{
					const ModelRenderFlags flags = instances.model->flags;
					renderQueue->culling(!any(flags & ModelRenderFlags::TwoSided));
					renderQueue->depthTest(any(flags & ModelRenderFlags::DepthTest));
					renderQueue->depthWrite(any(flags & ModelRenderFlags::DepthWrite));
				}
				if (instances.armatures.empty())
				{
					uint32 off = 0;
					while (off < instances.data.size())
					{
						const uint32 n = min(numeric_cast<uint32>(instances.data.size() - off), (uint32)CAGE_SHADER_MAX_INSTANCES);
						renderQueue->universalUniformArray(subRange<const UniModel>(instances.data, off, n), CAGE_SHADER_UNIBLOCK_MESHES);
						renderQueue->draw(n);
						off += n;
					}
					CAGE_ASSERT(off == instances.data.size());
				}
				else
					renderInstancesSkeletons(instances);
				renderQueue->checkGlErrorDebug();
			}

			void renderEffect(CameraPass &pass)
			{
				renderQueue->colorTexture(0, pass.texTarget);
#ifdef CAGE_DEBUG
				renderQueue->checkFrameBuffer();
#endif // CAGE_DEBUG
				renderQueue->bind(pass.texSource);
				//renderQueue->bind(modelSquare);
				renderQueue->draw();
				renderQueue->checkGlErrorDebug();
				std::swap(pass.texSource, pass.texTarget);
			}

			void renderCameraPass(CameraPass &pass)
			{
				const auto graphicsDebugScope = renderQueue->namedScope("camera pass");

				ScreenSpaceCommonConfig gfCommonConfig; // helper to simplify initialization
				gfCommonConfig.assets = engineAssets();
				gfCommonConfig.provisionals = +provisionalData;
				gfCommonConfig.queue = +renderQueue;
				gfCommonConfig.resolution = pass.resolution;

				renderQueue->universalUniformStruct(pass.viewport, CAGE_SHADER_UNIBLOCK_VIEWPORT);
				renderQueue->viewport(ivec2(), pass.resolution);

				{ // deferred
					const auto graphicsDebugScope = renderQueue->namedScope("deferred");

					{ // prepare gBuffer
						const auto graphicsDebugScope = renderQueue->namedScope("prepare gBuffer target");
						pass.albedoTexture = prepareGTexture("albedoTexture", GL_RGB8, pass.resolution);
						pass.specialTexture = prepareGTexture("specialTexture", GL_RG8, pass.resolution);
						pass.normalTexture = prepareGTexture("normalTexture", GL_RGB16F, pass.resolution);
						pass.colorTexture = prepareGTexture("colorTexture", GL_RGB16F, pass.resolution);
						pass.velocityTexture = prepareGTexture("velocityTexture", GL_RG16F, pass.resolution, any(pass.camera->camera.effects & CameraEffectsFlags::MotionBlur));
						pass.depthTexture = prepareGTexture("depthTexture", GL_DEPTH_COMPONENT32, pass.resolution);
						pass.intermediateTexture = prepareGTexture("intermediateTexture", GL_RGB16F, pass.resolution);
						pass.ambientOcclusionTexture = prepareGTexture("ambientOcclusionTexture", GL_R8, max(pass.resolution / CAGE_SHADER_SSAO_DOWNSCALE, 1u), any(pass.camera->camera.effects & CameraEffectsFlags::AmbientOcclusion));
						renderQueue->checkGlErrorDebug();
						pass.gBufferTarget = provisionalData->frameBufferDraw("gBuffer");
						renderQueue->bind(pass.gBufferTarget);
						renderQueue->colorTexture(CAGE_SHADER_ATTRIB_OUT_ALBEDO, pass.albedoTexture);
						renderQueue->colorTexture(CAGE_SHADER_ATTRIB_OUT_SPECIAL, pass.specialTexture);
						renderQueue->colorTexture(CAGE_SHADER_ATTRIB_OUT_NORMAL, pass.normalTexture);
						renderQueue->colorTexture(CAGE_SHADER_ATTRIB_OUT_COLOR, pass.colorTexture);
						renderQueue->colorTexture(CAGE_SHADER_ATTRIB_OUT_VELOCITY, any(pass.camera->camera.effects & CameraEffectsFlags::MotionBlur) ? pass.velocityTexture : TextureHandle());
						renderQueue->depthTexture(pass.depthTexture);
						renderQueue->activeAttachments(31);
						renderQueue->checkFrameBuffer();
						renderQueue->checkGlErrorDebug();

						// clear
						const auto cf = pass.camera->camera.clear;
						if (any(cf))
							renderQueue->clear(any(cf & CameraClearFlags::Color), any(cf & CameraClearFlags::Depth), any(cf & CameraClearFlags::Stencil));
					}

					{ // opaque objects
						renderQueue->culling(true);
						const auto graphicsDebugScope = renderQueue->namedScope("opaque objects");
						renderQueue->bind(shaderGBuffer);
						for (const auto &it : pass.opaque.data)
							renderInstances(it.second);
						renderQueue->resetAllState();
						renderQueue->checkGlErrorDebug();
					}

					{ // render target
						const auto graphicsDebugScope = renderQueue->namedScope("prepare render target");
						pass.renderTarget = provisionalData->frameBufferDraw("renderTarget");
						renderQueue->bind(pass.renderTarget);
						renderQueue->clearFrameBuffer();
						renderQueue->colorTexture(0, pass.colorTexture);
						renderQueue->activeAttachments(1);
						renderQueue->checkFrameBuffer();
						renderQueue->checkGlErrorDebug();
					}

					{ // bind gBuffer textures for sampling
						const auto graphicsDebugScope = renderQueue->namedScope("bind gBuffer textures");
						renderQueue->bind(pass.albedoTexture, CAGE_SHADER_TEXTURE_ALBEDO);
						renderQueue->bind(pass.specialTexture, CAGE_SHADER_TEXTURE_SPECIAL);
						renderQueue->bind(pass.normalTexture, CAGE_SHADER_TEXTURE_NORMAL);
						renderQueue->bind(pass.depthTexture, CAGE_SHADER_TEXTURE_DEPTH);
						renderQueue->bind(pass.colorTexture, CAGE_SHADER_TEXTURE_COLOR);
						renderQueue->activeTexture(CAGE_SHADER_TEXTURE_COLOR);
						renderQueue->checkGlErrorDebug();
					}

					{ // lights
						const auto graphicsDebugScope = renderQueue->namedScope("lighting");
						const auto &bindLightType = [&](LightTypeEnum type, bool shadows)
						{
							uint32 shaderRoutines[CAGE_SHADER_MAX_ROUTINES] = {};
							switch (type)
							{
							case LightTypeEnum::Directional:
								shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE] = shadows ? CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONALSHADOW : CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONAL;
								renderQueue->bind(modelSquare);
								renderQueue->cullFace(false);
								break;
							case LightTypeEnum::Spot:
								shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE] = shadows ? CAGE_SHADER_ROUTINEPROC_LIGHTSPOTSHADOW : CAGE_SHADER_ROUTINEPROC_LIGHTSPOT;
								renderQueue->bind(modelCone);
								renderQueue->cullFace(true);
								break;
							case LightTypeEnum::Point:
								shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE] = shadows ? CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW : CAGE_SHADER_ROUTINEPROC_LIGHTPOINT;
								renderQueue->bind(modelSphere);
								renderQueue->cullFace(true);
								break;
							default:
								CAGE_THROW_CRITICAL(Exception, "invalid light type");
							}
							renderQueue->uniform(CAGE_SHADER_UNI_ROUTINES, shaderRoutines);
						};
						renderQueue->blending(true);
						renderQueue->blendFuncAdditive();
						renderQueue->bind(shaderLighting);
						for (const auto &l : pass.lights.shadowed)
						{
							bindLightType(l.light->emit->light.lightType, true);
							renderQueue->bind(l.shadowmap->depthTexture, l.light->emit->light.lightType == LightTypeEnum::Point ? CAGE_SHADER_TEXTURE_SHADOW_CUBE : CAGE_SHADER_TEXTURE_SHADOW);
							renderQueue->universalUniformStruct<UniLight>(l.data, CAGE_SHADER_UNIBLOCK_LIGHTS);
							renderQueue->draw();
						}
						const std::pair<std::vector<UniLight> &, LightTypeEnum> types[] =
						{
							{ pass.lights.directional, LightTypeEnum::Directional },
							{ pass.lights.spot, LightTypeEnum::Spot },
							{ pass.lights.point, LightTypeEnum::Point },
						};
						for (const auto &t : types)
						{
							if (t.first.empty())
								continue;
							bindLightType(t.second, false);
							renderQueue->universalUniformArray<UniLight>(t.first, CAGE_SHADER_UNIBLOCK_LIGHTS);
							renderQueue->draw(numeric_cast<uint32>(t.first.size()));
						}
						renderQueue->resetAllState();
						renderQueue->checkGlErrorDebug();
					}
				}

				{ // opaque effects
					const auto graphicsDebugScope2 = renderQueue->namedScope("opaque effects");
					renderQueue->bind(pass.renderTarget);
					renderQueue->clearFrameBuffer();
					renderQueue->activeAttachments(1);
					pass.texSource = pass.colorTexture;
					pass.texTarget = pass.intermediateTexture;

					const CameraEffectsFlags effects = pass.camera->camera.effects;
					const auto &params = pass.camera->camera;

					// ssao
					if (any(effects & CameraEffectsFlags::AmbientOcclusion))
					{
						ScreenSpaceAmbientOcclusionConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
						(ScreenSpaceAmbientOcclusion &)cfg = params.ssao;
						cfg.frameIndex = frameIndex;
						cfg.inDepth = pass.depthTexture;
						cfg.inNormal = pass.normalTexture;
						cfg.outAo = pass.ambientOcclusionTexture;
						cfg.viewProj = pass.viewProj;
						screenSpaceAmbientOcclusion(cfg);
					}

					// ambient light
					if ((pass.viewport.ambientLight + pass.viewport.ambientDirectionalLight) != vec4())
					{
						const auto graphicsDebugScope = renderQueue->namedScope("ambient light");
						renderQueue->viewport(ivec2(), pass.resolution);
						renderQueue->bind(shaderAmbient);
						renderQueue->bind(modelSquare);
						if (any(effects & CameraEffectsFlags::AmbientOcclusion))
						{
							renderQueue->bind(pass.ambientOcclusionTexture, CAGE_SHADER_TEXTURE_EFFECTS);
							renderQueue->activeTexture(CAGE_SHADER_TEXTURE_COLOR);
							renderQueue->uniform(CAGE_SHADER_UNI_AMBIENTOCCLUSION, 1);
						}
						else
							renderQueue->uniform(CAGE_SHADER_UNI_AMBIENTOCCLUSION, 0);
						renderEffect(pass);
					}

					// depth of field
					if (any(effects & CameraEffectsFlags::DepthOfField))
					{
						ScreenSpaceDepthOfFieldConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
						(ScreenSpaceDepthOfField &)cfg = params.depthOfField;
						cfg.proj = pass.proj;
						cfg.inDepth = pass.depthTexture;
						cfg.inColor = pass.texSource;
						cfg.outColor = pass.texTarget;
						screenSpaceDepthOfField(cfg);
						std::swap(pass.texSource, pass.texTarget);
					}

					// motion blur
					if (any(effects & CameraEffectsFlags::MotionBlur))
					{
						ScreenSpaceMotionBlurConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
						(ScreenSpaceMotionBlur &)cfg = params.motionBlur;
						cfg.inVelocity = pass.velocityTexture;
						cfg.inColor = pass.texSource;
						cfg.outColor = pass.texTarget;
						screenSpaceMotionBlur(cfg);
						std::swap(pass.texSource, pass.texTarget);
					}

					// eye adaptation
					if (any(effects & CameraEffectsFlags::EyeAdaptation))
					{
						ScreenSpaceEyeAdaptationConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
						(ScreenSpaceEyeAdaptation &)cfg = params.eyeAdaptation;
						cfg.cameraId = stringizer() + pass.camera->emit->entityId;
						cfg.inColor = pass.texSource;
						screenSpaceEyeAdaptationPrepare(cfg);
					}

					renderQueue->resetAllState();
					renderQueue->viewport(ivec2(), pass.resolution);
					renderQueue->bind(pass.renderTarget);
					renderQueue->bind(pass.texSource, CAGE_SHADER_TEXTURE_COLOR);
				}

				//renderCameraTransparencies(pass);
				//renderCameraEffectsFinal(pass);

				{ // blit to destination
					const auto graphicsDebugScope = renderQueue->namedScope("blit to destination");
					renderQueue->resetFrameBuffer();
					renderQueue->bind(pass.texSource, 0);
					renderQueue->depthTest(false);
					renderQueue->depthWrite(false);
					if (pass.camera->target)
					{ // blit to target texture
						renderQueue->colorTexture(0, pass.camera->target);
						renderQueue->viewport(ivec2(), pass.resolution);
						renderQueue->bind(shaderBlit);
					}
					else
					{ // blit to window
						renderQueue->viewport(ivec2(), windowResolution);
						renderQueue->bind(shaderVisualizeColor);
						renderQueue->uniform(0, 1.0 / vec2(windowResolution));
					}
					renderQueue->bind(modelSquare);
					renderQueue->draw();
					renderQueue->resetAllState();
					renderQueue->resetAllTextures();
					renderQueue->resetFrameBuffer();
					renderQueue->checkGlErrorDebug();
				}
			}

			void run()
			{
				if (windowResolution[0] <= 0 || windowResolution[1] <= 0)
					return;
				if (!loadGeneralAssets())
					return;

				copyEmitToPrep();
				updateCommonValues();

				// sort cameras
				std::sort(cameras.begin(), cameras.end(), [](const PrepCamera &aa, const PrepCamera &bb) {
					const auto a = aa.camera.cameraOrder;
					const auto b = bb.camera.cameraOrder;
					CAGE_ASSERT(a != b);
					return a < b;
				});

				for (const auto &it : cameras)
				{
					CameraPass cam;
					cam.camera = &it;
					generateCameraPass(cam);
					renderCameraPass(cam);
				}
			}
		};
	}

	void Graphics::prepare(uint64 time)
	{
		renderQueue->resetQueue();

		if (auto lock = emitBuffersGuard->read())
		{
			Preparator prep(emitBuffers[lock.index()], time);
			prep.run();
		}

		renderQueue->resetFrameBuffer();
		renderQueue->resetAllTextures();
		renderQueue->resetAllState();

		outputDrawCalls = renderQueue->drawsCount();
		outputDrawPrimitives = renderQueue->primitivesCount();
	}
}
