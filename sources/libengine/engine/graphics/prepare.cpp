#include <cage-core/pointerRangeHolder.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-core/concurrent.h>
#include <cage-core/hashString.h>
#include <cage-core/profiling.h>
#include <cage-core/geometry.h>
#include <cage-core/camera.h>
#include <cage-core/config.h>
#include <cage-core/color.h>
#include <cage-core/tasks.h>

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
#include "../../blockCollection.h"

#include <robin_hood.h>
#include <algorithm>

namespace cage
{
	// implemented in gui
	String loadInternationalizedText(AssetManager *assets, uint32 asset, uint32 text, String params);

	namespace
	{
		ConfigSint32 confVisualizeBuffer("cage/graphics/visualizeBuffer", 0);
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
			Vec4 data[3];

			Mat3x4() : Mat3x4(Mat4())
			{}

			explicit Mat3x4(const Mat3 &in)
			{
				for (uint32 a = 0; a < 3; a++)
					for (uint32 b = 0; b < 3; b++)
						data[b][a] = in[a * 3 + b];
			}

			explicit Mat3x4(const Mat4 &in)
			{
				CAGE_ASSERT(in[3] == 0 && in[7] == 0 && in[11] == 0 && in[15] == 1);
				for (uint32 a = 0; a < 4; a++)
					for (uint32 b = 0; b < 3; b++)
						data[b][a] = in[a * 4 + b];
			}
		};

		struct UniModel
		{
			Mat4 mvpMat;
			Mat4 mvpMatPrev;
			Mat3x4 normalMat; // [2][3] is 1 if lighting is enabled and 0 otherwise
			Mat3x4 mMat;
			Vec4 color; // color rgb is linear, and NOT alpha-premultiplied
			Vec4 aniTexFrames;
		};

		struct UniLight
		{
			Mat4 shadowMat;
			Mat4 mvpMat;
			Vec4 color; // + spotAngle
			Vec4 position;
			Vec4 direction; // + normalOffsetScale
			Vec4 attenuation; // + spotExponent
		};

		struct UniViewport
		{
			Mat4 vpInv;
			Vec4 eyePos;
			Vec4 eyeDir;
			Vec4 viewport; // x, y, w, h
			Vec4 ambientLight;
			Vec4 ambientDirectionalLight;
		};

		struct PrepTransform
		{
			Mat4 model;
			Mat4 modelPrev;
		};

		struct PrepSkeleton : private Immovable
		{
			Holder<const SkeletonRig> rig;
			Holder<const SkeletalAnimation> animation;
			SkeletalAnimationComponent params;
			std::vector<Mat3x4> armature;
			Real coefficient = Real::Nan();
			Holder<AsyncTask> task;

			void operator () (uint32)
			{
				Mat4 tmpArmature[CAGE_SHADER_MAX_BONES];
				const uint32 bonesCount = rig->bonesCount();
				CAGE_ASSERT(bonesCount > 0);
				CAGE_ASSERT(animation->bonesCount() == bonesCount);
				animateSkin(+rig, +animation, coefficient, tmpArmature);
				CAGE_ASSERT(armature.empty());
				armature.reserve(bonesCount);
				for (uint32 i = 0; i < bonesCount; i++)
					armature.emplace_back(tmpArmature[i]);
			}
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
			Vec3 center;

			PrepRender(const EmitRender *e) : emit(e)
			{}
		};

		struct PrepText : public PrepTransform
		{
			const EmitText *emit = nullptr;
			TextComponent text;
			Holder<const Font> font;
			std::vector<uint32> glyphs;
			FontFormat format;
			Vec3 color;

			PrepText(const EmitText *e) : emit(e)
			{}
		};

		struct PrepLight : public PrepTransform
		{
			const EmitLight *emit = nullptr;
			LightComponent light;
			Mat4 mMat; // light model matrix
			Aabb shape; // world-space area of influence - used for culling

			PrepLight(const EmitLight *e) : emit(e)
			{}
		};

		struct PrepCamera : public PrepTransform
		{
			const EmitCamera *emit = nullptr;
			CameraComponent camera;
			Holder<Texture> target;
			Vec3 center;

			PrepCamera(const EmitCamera *e) : emit(e)
			{}
		};

		struct RendersInstances : private Noncopyable
		{
			BlockCollection<UniModel> data;
			BlockCollection<PrepSkeleton *> armatures;
			Holder<Model> model;
			Holder<Texture> textures[MaxTexturesCountPerMaterial];
			uint32 shaderRoutines[CAGE_SHADER_MAX_ROUTINES] = {};
		};

		struct RendersCollection : private Noncopyable
		{
			struct Key
			{
				const Model *model = nullptr;
				bool skeletalAnimation = false;
			};
			struct Hasher
			{
				auto operator () (const Key &k) const
				{
					return std::hash<const Model *>()(k.model) + k.skeletalAnimation;
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
			std::vector<const PrepText *> data;
		};

		struct TextsCollection : private Noncopyable
		{
			robin_hood::unordered_map<const Font *, TextsInstances> data;
		};

		struct ShadowedLight
		{
			const PrepLight *light = nullptr;
			UniLight data;
			const struct ShadowmapPass *shadowmap = nullptr;
		};

		struct LightsCollection : private Noncopyable
		{
			std::vector<ShadowedLight> shadowed;
			BlockCollection<UniLight> directional, spot, point;

			LightsCollection() : directional(CAGE_SHADER_MAX_INSTANCES), spot(CAGE_SHADER_MAX_INSTANCES), point(CAGE_SHADER_MAX_INSTANCES)
			{}
		};

		struct TranslucentsInstances : private Noncopyable
		{
			RendersInstances renders;
			LightsCollection lights;
			Aabb box; // world-space box for the renders - used for culling lights
			Real distToCam2; // used for sorting back-to-front
		};

		struct TranslucentsCollection : private Noncopyable
		{
			std::vector<TranslucentsInstances> data;
		};

		struct CommonRenderPass : private Noncopyable
		{
			const PrepCamera *camera = nullptr;
			Mat4 view;
			Mat4 viewPrev;
			Mat4 proj;
			Mat4 viewProj;
			Mat4 viewProjPrev;
			Vec2i resolution;

			struct LodSelection
			{
				Vec3 center; // center of camera (NOT light)
				Real screenSize = 0; // vertical size of screen in pixels, one meter in front of the camera
				bool orthographic = false;
			} lodSelection;

			Holder<AsyncTask> task;
		};

		struct ShadowmapPass : public CommonRenderPass
		{
			const PrepLight *light = nullptr;
			RendersCollection shadowers;
			Mat4 shadowMat;
			TextureHandle shadowTexture;
		};

		struct FilteredLight
		{
			const PrepLight *light = nullptr;
			ShadowmapPass *shadowmap = nullptr;
		};

		struct CameraPass : public CommonRenderPass
		{
			UniViewport viewport;
			std::vector<Holder<ShadowmapPass>> shadowmapPasses;
			std::vector<FilteredLight> filteredLights;
			RendersCollection opaque;
			LightsCollection lights;
			TranslucentsCollection translucents;
			TextsCollection texts;
		};

		struct Preparator
		{
			const EmitBuffer &emit;
			const Holder<RenderQueue> &renderQueue = graphics->renderQueue;
			const Holder<ProvisionalGraphics> &provisionalData = graphics->provisionalData;
			const uint32 frameIndex = graphics->frameIndex;
			const bool cnfRenderMissingModels = confRenderMissingModels;
			const bool cnfRenderSkeletonBones = confRenderSkeletonBones;

			Holder<Mutex> skeletonTaskInitMutex = newMutex();

			uint64 prepareTime = 0;
			Real interFactor;
			Vec2i windowResolution;

			Holder<Model> modelSquare, modelSphere, modelCone, modelBone;
			Holder<ShaderProgram> shaderAmbient, shaderBlit, shaderDepth, shaderGBuffer, shaderLighting, shaderTranslucent;
			Holder<ShaderProgram> shaderVisualizeColor, shaderVisualizeDepth, shaderVisualizeMonochromatic, shaderVisualizeVelocity;
			Holder<ShaderProgram> shaderFont;

			std::vector<PrepRender> renders;
			std::vector<PrepText> texts;
			std::vector<PrepLight> lights;
			std::vector<PrepCamera> cameras;

			std::vector<CameraPass> cameraPasses;

			Preparator(const EmitBuffer &emit, uint64 time) : emit(emit)
			{
				prepareTime = graphics->itc(emit.time, time, controlThread().updatePeriod());
				interFactor = saturate(Real(prepareTime - emit.time) / controlThread().updatePeriod());
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
				shaderVisualizeDepth = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/visualize/depth.glsl"));
				shaderVisualizeMonochromatic = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/visualize/monochromatic.glsl"));
				shaderVisualizeVelocity = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/visualize/velocity.glsl"));
				shaderFont = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/font.glsl"));
				CAGE_ASSERT(shaderBlit);
				return true;
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

				pr.center = Vec3(pr.model * Vec4(0, 0, 0, 1));

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
					pr.render.color = Vec3(0);
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
					if (ps->animation)
						ps->rig = engineAssets()->tryGet<AssetSchemeIndexSkeletonRig, SkeletonRig>(ps->animation->skeletonName());
					if (ps->animation && ps->rig)
						ps->coefficient = detail::evalCoefficientForSkeletalAnimation(+ps->animation, prepareTime, ps->params.startTime, ps->params.speed, ps->params.offset);
					else
						ps.clear();
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

				const String str = loadInternationalizedText(engineAssets(), pt.text.assetName, pt.text.textName, pt.text.value);
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
				const Vec2 size = pt.font->size(pt.glyphs, pt.format);
				pt.format.wrapWidth = size[0];
				const Mat4 tr = Mat4(Vec3(size * Vec2(-0.5, 0.5), 0));
				pt.model *= tr;
				pt.modelPrev *= tr;
			}

			static Real lightRange(const Vec3 &color, const Vec3 &attenuation)
			{
				Real c = max(color[0], max(color[1], color[2]));
				if (c <= 1e-5)
					return 0;
				Real e = c * 100;
				Real x = attenuation[0], y = attenuation[1], z = attenuation[2];
				if (z < 1e-5)
				{
					CAGE_ASSERT(y > 1e-5);
					return (e - x) / y;
				}
				return (sqrt(y * y - 4 * z * (x - e)) - y) / (2 * z);
			}

			static Mat4 spotConeModelMatrix(Real range, Rads angle)
			{
				Real xy = tan(angle * 0.5);
				Vec3 scale = Vec3(xy, xy, 1) * range;
				return Mat4(Vec3(), Quat(), scale);
			}

			void updateCommonValues(PrepLight &pl)
			{
				pl.light = pl.emit->light;

				switch (pl.light.lightType)
				{
				case LightTypeEnum::Directional:
					pl.mMat = Mat4(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, -1, -1, 0, 1); // full screen quad
					pl.shape = Aabb::Universe();
					break;
				case LightTypeEnum::Spot:
				{
					const Real range = lightRange(pl.light.color, pl.light.attenuation);
					pl.mMat = pl.model * spotConeModelMatrix(range, pl.light.spotAngle);
					pl.shape = Aabb(Cone(Vec3(pl.model * Vec4(0, 0, 0, 1)), normalize(Vec3(pl.model * Vec4(0, 0, -1, 0))), range, pl.light.spotAngle));
				} break;
				case LightTypeEnum::Point:
					pl.mMat = pl.model * Mat4::scale(lightRange(pl.light.color, pl.light.attenuation));
					pl.shape = Aabb(Vec3(-1), Vec3(1)) * pl.mMat;
					break;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid light type");
				}
			}

			void updateCommonValues(PrepCamera &pc)
			{
				pc.camera = pc.emit->camera;
				pc.center = Vec3(pc.model * Vec4(0, 0, 0, 1));

				if (confNoAmbientOcclusion)
					pc.camera.effects &= ~CameraEffectsFlags::AmbientOcclusion;
				if (confNoMotionBlur)
					pc.camera.effects &= ~CameraEffectsFlags::MotionBlur;
				if (confNoBloom)
					pc.camera.effects &= ~CameraEffectsFlags::Bloom;

				if (pc.camera.target)
					pc.target = Holder<Texture>(pc.camera.target, nullptr);
			}

			template<class E, class P>
			void copyEmitToPrep(const std::vector<E> &es, std::vector<P> &ps)
			{
				ps.reserve(es.size());
				for (const auto &it_ : es)
				{
					ps.emplace_back(&it_);
					auto &it = ps.back();
					const Transform &a = it.emit->history;
					const Transform &b = it.emit->current;
					it.model = Mat4(interpolate(a, b, interFactor));
					it.modelPrev = Mat4(interpolate(a, b, interFactor - 0.2));
					updateCommonValues(it);
				}
			}

			void copyEmitToPrep()
			{
				ProfilingScope profiling("copy emit to prep", "graphics");
				copyEmitToPrep(emit.renders, renders);
				copyEmitToPrep(emit.texts, texts);
				copyEmitToPrep(emit.lights, lights);
				copyEmitToPrep(emit.cameras, cameras);
			}

			void addModels(const CommonRenderPass &pass, const PrepRender &pr, Holder<Model> mo, RendersInstances &instances)
			{
				if (!instances.model)
				{
					CAGE_ASSERT(instances.data.empty());
					CAGE_ASSERT(instances.armatures.empty());
					if (pr.skeletalAnimation)
					{
						const uint32 drawLimit = min((uint32)CAGE_SHADER_MAX_INSTANCES, CAGE_SHADER_MAX_BONES / mo->skeletonBones);
						instances.data = BlockCollection<UniModel>(drawLimit);
						instances.armatures = BlockCollection<PrepSkeleton *>(drawLimit);
					}
					else
					{
						instances.data = BlockCollection<UniModel>(CAGE_SHADER_MAX_INSTANCES);
						instances.armatures = BlockCollection<PrepSkeleton *>(0); // disabled
					}

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

				UniModel um;
				um.color = Vec4(colorGammaToLinear(pr.render.color) * pr.render.intensity, pr.render.opacity);
				um.mMat = Mat3x4(pr.model);
				um.mvpMat = pass.viewProj * pr.model;
				um.mvpMatPrev = any(mo->flags & ModelRenderFlags::VelocityWrite) ? pass.viewProjPrev * pr.modelPrev : um.mvpMat;
				um.normalMat = Mat3x4(inverse(Mat3(pr.model)));
				um.normalMat.data[2][3] = any(mo->flags & ModelRenderFlags::Lighting) ? 1 : 0; // is lighting enabled

				if (pr.textureAnimation)
				{
					Holder<Texture> t = engineAssets()->tryGet<AssetSchemeIndexTexture, Texture>(mo->textureName(CAGE_SHADER_TEXTURE_ALBEDO));
					if (t)
					{
						const auto &p = pr.textureAnimation->params;
						um.aniTexFrames = detail::evalSamplesForTextureAnimation(+t, prepareTime, p.startTime, p.speed, p.offset);
					}
				}

				instances.data.push_back(um);

				if (pr.skeletalAnimation)
				{
					{
						ScopeLock lock(skeletonTaskInitMutex);
						if (!pr.skeletalAnimation->task)
							pr.skeletalAnimation->task = tasksRunAsync("skeletal-animation", Holder<PrepSkeleton>(+pr.skeletalAnimation, nullptr)); // the task may not own the skeleton otherwise it would make a cyclic dependency
					}
					instances.armatures.push_back(+pr.skeletalAnimation);
					CAGE_ASSERT(instances.armatures.size() == instances.data.size());
				}
			}

			template<bool ShadowCastersOnly>
			void addModels(const CommonRenderPass &pass, const PrepRender &pr, Holder<Model> mo, RendersCollection &opaque, TranslucentsCollection &translucents)
			{
				if constexpr (ShadowCastersOnly)
				{
					if (none(mo->flags & ModelRenderFlags::ShadowCast))
						return;
				}

				if (!intersects(mo->boundingBox(), Frustum(pass.viewProj * pr.model)))
					return;

				if constexpr (!ShadowCastersOnly)
				{
					if (any(mo->flags & ModelRenderFlags::Translucent) || pr.render.opacity < 1)
					{
						translucents.data.emplace_back();
						translucents.data.back().box = mo->boundingBox() * pr.model;
						translucents.data.back().distToCam2 = distanceSquared(pass.camera->center, pr.center);
						addModels(pass, pr, std::move(mo), translucents.data.back().renders);
						return;
					}
				}

				addModels(pass, pr, mo.share(), opaque.data[{ +mo, !!pr.skeletalAnimation }]);
			}

			template<bool ShadowCastersOnly>
			void addModelsSkeleton(const CommonRenderPass &pass, const PrepRender &pr, RendersCollection &opaque, TranslucentsCollection &translucents)
			{
				Mat4 tmpArmature[CAGE_SHADER_MAX_BONES];
				const auto &s = pr.skeletalAnimation->rig;
				const auto &a = pr.skeletalAnimation->animation;
				const auto &p = pr.skeletalAnimation->params;
				const uint32 bonesCount = s->bonesCount();
				animateSkeleton(+s, +a, pr.skeletalAnimation->coefficient, tmpArmature);
				for (uint32 i = 0; i < bonesCount; i++)
				{
					PrepRender r(pr.emit);
					r.model = pr.model * tmpArmature[i];
					r.modelPrev = pr.modelPrev * tmpArmature[i];
					r.render = pr.render;
					r.render.color = colorHsvToRgb(Vec3(Real(i) / Real(bonesCount), 1, 1));
					r.render.object = 0;
					addModels<ShadowCastersOnly>(pass, r, modelBone.share(), opaque, translucents);
				}
			}

			template<bool ShadowCastersOnly>
			void addModels(const CommonRenderPass &pass, const PrepRender &pr, RendersCollection &opaque, TranslucentsCollection &translucents)
			{
				CAGE_ASSERT(pass.lodSelection.screenSize > 0);
				CAGE_ASSERT(pr.render.object);

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
						Real d = 1;
						if (!pass.lodSelection.orthographic)
						{
							const Vec4 ep4 = pr.model * Vec4(0, 0, 0, 1);
							CAGE_ASSERT(abs(ep4[3] - 1) < 1e-4);
							d = distance(Vec3(ep4), pass.lodSelection.center);
						}
						const Real f = pass.lodSelection.screenSize * pr.object->worldSize / (d * pr.object->pixelsSize);
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

			void addLight(const PrepLight &pl, const Mat4 &viewProj, const ShadowmapPass *shadowmap, LightsCollection &outputs)
			{
				UniLight ul;
				if (pl.light.lightType == LightTypeEnum::Directional)
					ul.mvpMat = pl.mMat;
				else
					ul.mvpMat = viewProj * pl.mMat;
				ul.color = Vec4(colorGammaToLinear(pl.light.color) * pl.light.intensity, cos(pl.light.spotAngle * 0.5));
				ul.attenuation = Vec4(pl.light.attenuation, pl.light.spotExponent);
				ul.direction = Vec4(normalize(Vec3(pl.model * Vec4(0, 0, -1, 0))), shadowmap ? pl.emit->shadowmap->normalOffsetScale : 0);
				ul.position = pl.model * Vec4(0, 0, 0, 1);

				if (shadowmap)
				{
					ul.shadowMat = shadowmap->shadowMat;
					ShadowedLight sl;
					sl.data = ul;
					sl.shadowmap = shadowmap;
					sl.light = &pl;
					outputs.shadowed.push_back(sl);
				}
				else
				{
					switch (pl.light.lightType)
					{
					case LightTypeEnum::Directional:
						outputs.directional.push_back(ul);
						break;
					case LightTypeEnum::Spot:
						outputs.spot.push_back(ul);
						break;
					case LightTypeEnum::Point:
						outputs.point.push_back(ul);
						break;
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid light type");
					}
				}
			}

			void addLight(CameraPass &pass, const PrepLight &pl, const ShadowmapPass *shadowmap)
			{
				addLight(pl, pass.viewProj, shadowmap, pass.lights);
			}

			void addLight(TranslucentsInstances &translucents, const PrepLight &pl, const Mat4 &viewProj, const ShadowmapPass *shadowmap)
			{
				addLight(pl, viewProj, shadowmap, translucents.lights);
			}

			static void initializeLodSelection(CommonRenderPass &pass)
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
				pass.lodSelection.center = Vec3(pass.camera->model * Vec4(0, 0, 0, 1));
			}

			void initializeShadowmapPass(ShadowmapPass &pass)
			{
				ProfilingScope profiling("initialize shadowmap pass", "graphics");

				const LightComponent &lc = pass.light->light;
				const ShadowmapComponent &sc = *pass.light->emit->shadowmap;

				pass.resolution = Vec2i(sc.resolution);
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
					pass.proj = perspectiveProjection(Degs(90), 1, sc.worldSize[0], sc.worldSize[1]);
					break;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid light type");
				}
				pass.viewProj = pass.viewProjPrev = pass.proj * pass.view;

				constexpr Mat4 bias = Mat4(
					0.5, 0.0, 0.0, 0.0,
					0.0, 0.5, 0.0, 0.0,
					0.0, 0.0, 0.5, 0.0,
					0.5, 0.5, 0.5, 1.0);
				pass.shadowMat = bias * pass.viewProj;

				initializeLodSelection(pass);
			}

			void generateShadowmapPass(ShadowmapPass &pass, uint32)
			{
				ProfilingScope profiling("generate shadowmap pass", "graphics");

				const uint32 sceneMask = pass.camera->camera.sceneMask;
				for (const PrepRender &pr : renders)
				{
					if ((pr.render.sceneMask & sceneMask) == 0 || !pr.render.object)
						continue;
					addModels(pass, pr);
				}
			}

			void initializeCameraPass(CameraPass &pass, uint32)
			{
				ProfilingScope profiling("initialize camera pass", "graphics");

				const CameraComponent &cc = pass.camera->camera;

				pass.resolution = pass.camera->target ? pass.camera->target->resolution() : windowResolution;
				pass.view = inverse(pass.camera->model);
				pass.viewPrev = inverse(pass.camera->modelPrev);
				switch (cc.cameraType)
				{
				case CameraTypeEnum::Orthographic:
				{
					const Vec2 &os = cc.camera.orthographicSize;
					pass.proj = orthographicProjection(-os[0], os[0], -os[1], os[1], cc.near, cc.far);
				} break;
				case CameraTypeEnum::Perspective:
					pass.proj = perspectiveProjection(cc.camera.perspectiveFov, Real(pass.resolution[0]) / Real(pass.resolution[1]), cc.near, cc.far);
					break;
				default:
					CAGE_THROW_ERROR(Exception, "invalid camera type");
				}
				pass.viewProj = pass.proj * pass.view;
				pass.viewProjPrev = pass.proj * pass.viewPrev;

				pass.viewport.vpInv = inverse(pass.viewProj);
				pass.viewport.eyePos = pass.camera->model * Vec4(0, 0, 0, 1);
				pass.viewport.eyeDir = pass.camera->model * Vec4(0, 0, -1, 0);
				pass.viewport.ambientLight = Vec4(colorGammaToLinear(cc.ambientColor) * cc.ambientIntensity, 0);
				pass.viewport.ambientDirectionalLight = Vec4(colorGammaToLinear(cc.ambientDirectionalColor) * cc.ambientDirectionalIntensity, 0);
				pass.viewport.viewport = Vec4(Vec2(), Vec2(pass.resolution));

				initializeLodSelection(pass);

				pass.filteredLights.reserve(lights.size());
				for (const PrepLight &pl : lights)
				{
					if ((pl.light.sceneMask & cc.sceneMask) == 0)
						continue;
					if (!intersects(pl.shape, Frustum(pass.viewProj)))
						continue;

					ShadowmapPass *shadowmap = nullptr;
					if (pl.emit->shadowmap)
					{
						Holder<ShadowmapPass> sp = systemMemory().createHolder<ShadowmapPass>();
						sp->light = &pl;
						sp->camera = pass.camera;
						initializeShadowmapPass(*sp);
						shadowmap = +sp;
						pass.shadowmapPasses.push_back(std::move(sp));
					}

					pass.filteredLights.push_back(FilteredLight{ &pl, shadowmap });
				}
			}

			void generateCameraPass(CameraPass &pass, uint32)
			{
				ProfilingScope profiling("generate camera pass", "graphics");

				const CameraComponent &cc = pass.camera->camera;

				for (const PrepRender &pr : renders)
				{
					if ((pr.render.sceneMask & cc.sceneMask) == 0 || !pr.render.object)
						continue;
					addModels(pass, pr);
				}

				std::sort(pass.translucents.data.begin(), pass.translucents.data.end(), [](const TranslucentsInstances &a, const TranslucentsInstances &b) { return a.distToCam2 > b.distToCam2; });

				for (const PrepText &pt : texts)
				{
					if ((pt.text.sceneMask & cc.sceneMask) == 0 || !pt.font)
						continue;

					// todo frustum culling

					pass.texts.data[+pt.font].data.push_back(&pt);
				}

				for (const auto &pl : pass.filteredLights)
				{
					addLight(pass, *pl.light, pl.shadowmap);
					for (TranslucentsInstances &ti : pass.translucents.data)
					{
						if (!intersects(pl.light->shape, ti.box))
							continue;
						addLight(ti, *pl.light, pass.viewProj, pl.shadowmap);
					}
				}
			}

			TextureHandle prepareGTexture(const char *name, uint32 internalFormat, const Vec2i &resolution, bool enabled = true)
			{
				if (!enabled)
					return {};
				TextureHandle t = provisionalData->texture(Stringizer() + name + "_" + resolution[0] + "_" + resolution[1]);
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
				auto ait = instances.armatures.begin();
				for (const auto &it : instances.data)
				{
					const uint32 n = it.size();
					CAGE_ASSERT(n <= drawLimit);
					for (uint32 i = 0; i < n; i++)
					{
						const PrepSkeleton *sk = (*ait)[i];
						CAGE_ASSERT(sk && sk->task);
						sk->task->wait();
						CAGE_ASSERT(sk->armature.size() == bones);
						detail::memcpy(tmpArmature + i * bones, sk->armature.data(), bones * sizeof(Mat3x4));
					}
					renderQueue->universalUniformArray(subRange<const Mat3x4>(tmpArmature, 0, n * bones), CAGE_SHADER_UNIBLOCK_ARMATURES);
					renderQueue->universalUniformArray<UniModel>(it, CAGE_SHADER_UNIBLOCK_MESHES);
					renderQueue->draw(n);
					ait++;
				}
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
					for (const auto &it : instances.data)
					{
						renderQueue->universalUniformArray<UniModel>(it, CAGE_SHADER_UNIBLOCK_MESHES);
						renderQueue->draw(it.size());
					}
				}
				else
					renderInstancesSkeletons(instances);
				renderQueue->checkGlErrorDebug();
			}

			void renderShadowmapPass(ShadowmapPass &pass)
			{
				const auto graphicsDebugScope = renderQueue->namedScope("shadowmap pass");

				FrameBufferHandle renderTarget = provisionalData->frameBufferDraw("renderTarget");

				const String texName = Stringizer() + "shadowmap " + pass.light->emit->entityId;
				if (pass.light->light.lightType == LightTypeEnum::Point)
				{
					pass.shadowTexture = provisionalData->textureCube(texName);
					renderQueue->bind(pass.shadowTexture, CAGE_SHADER_TEXTURE_DEPTH);
					renderQueue->imageCube(pass.resolution, GL_DEPTH_COMPONENT16);
				}
				else
				{
					pass.shadowTexture = provisionalData->texture(texName);
					renderQueue->bind(pass.shadowTexture, CAGE_SHADER_TEXTURE_DEPTH);
					renderQueue->image2d(pass.resolution, GL_DEPTH_COMPONENT24);
				}
				renderQueue->filters(GL_LINEAR, GL_LINEAR, 16);
				renderQueue->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
				renderQueue->checkGlErrorDebug();

				renderQueue->bind(renderTarget);
				renderQueue->clearFrameBuffer();
				renderQueue->depthTexture(pass.shadowTexture);
				renderQueue->activeAttachments(0);
				renderQueue->checkFrameBuffer();
				renderQueue->viewport(Vec2i(), pass.resolution);
				renderQueue->colorWrite(false);
				renderQueue->clear(false, true);
				renderQueue->bind(shaderDepth);
				renderQueue->checkGlErrorDebug();

				for (const auto &it: pass.shadowers.data)
					renderInstances(it.second);

				renderQueue->resetAllState();
				renderQueue->resetAllTextures();
				renderQueue->resetFrameBuffer();
				renderQueue->checkGlErrorDebug();
			}

			template<class Binder>
			void renderLightsCollection(const LightsCollection &lights, const Binder &bindLightType)
			{
				for (const auto &l : lights.shadowed)
				{
					bindLightType(l.light->light.lightType, true);
					renderQueue->bind(l.shadowmap->shadowTexture, l.light->light.lightType == LightTypeEnum::Point ? CAGE_SHADER_TEXTURE_SHADOW_CUBE : CAGE_SHADER_TEXTURE_SHADOW);
					renderQueue->universalUniformStruct<UniLight>(l.data, CAGE_SHADER_UNIBLOCK_LIGHTS);
					renderQueue->draw();
				}

				const std::pair<const BlockCollection<UniLight> &, LightTypeEnum> types[] =
				{
					{ lights.directional, LightTypeEnum::Directional },
					{ lights.spot, LightTypeEnum::Spot },
					{ lights.point, LightTypeEnum::Point },
				};
				for (const auto &t : types)
				{
					if (t.first.empty())
						continue;
					bindLightType(t.second, false);
					for (const auto &it : t.first)
					{
						renderQueue->universalUniformArray<UniLight>(it, CAGE_SHADER_UNIBLOCK_LIGHTS);
						renderQueue->draw(it.size());
					}
				}
			}

			void renderCameraPass(CameraPass &pass)
			{
				const auto graphicsDebugScope = renderQueue->namedScope("camera pass");

				FrameBufferHandle gBufferTarget = provisionalData->frameBufferDraw("gBuffer");
				FrameBufferHandle renderTarget = provisionalData->frameBufferDraw("renderTarget");

				TextureHandle albedoTexture;
				TextureHandle specialTexture;
				TextureHandle normalTexture;
				TextureHandle depthTexture;
				TextureHandle colorTexture;
				TextureHandle intermediateTexture;
				TextureHandle velocityTexture;
				TextureHandle ambientOcclusionTexture;

				{ // prepare gBuffer
					const auto graphicsDebugScope = renderQueue->namedScope("prepare gBuffer target");
					albedoTexture = prepareGTexture("albedoTexture", GL_RGB8, pass.resolution);
					specialTexture = prepareGTexture("specialTexture", GL_RG8, pass.resolution);
					normalTexture = prepareGTexture("normalTexture", GL_RGB16F, pass.resolution);
					colorTexture = prepareGTexture("colorTexture", GL_RGB16F, pass.resolution);
					velocityTexture = prepareGTexture("velocityTexture", GL_RG16F, pass.resolution, any(pass.camera->camera.effects & CameraEffectsFlags::MotionBlur));
					depthTexture = prepareGTexture("depthTexture", GL_DEPTH_COMPONENT32, pass.resolution);
					intermediateTexture = prepareGTexture("intermediateTexture", GL_RGB16F, pass.resolution);
					ambientOcclusionTexture = prepareGTexture("ambientOcclusionTexture", GL_R8, max(pass.resolution / CAGE_SHADER_SSAO_DOWNSCALE, 1u), any(pass.camera->camera.effects & CameraEffectsFlags::AmbientOcclusion));
					renderQueue->checkGlErrorDebug();
					renderQueue->bind(gBufferTarget);
					renderQueue->colorTexture(CAGE_SHADER_ATTRIB_OUT_ALBEDO, albedoTexture);
					renderQueue->colorTexture(CAGE_SHADER_ATTRIB_OUT_SPECIAL, specialTexture);
					renderQueue->colorTexture(CAGE_SHADER_ATTRIB_OUT_NORMAL, normalTexture);
					renderQueue->colorTexture(CAGE_SHADER_ATTRIB_OUT_COLOR, colorTexture);
					renderQueue->colorTexture(CAGE_SHADER_ATTRIB_OUT_VELOCITY, any(pass.camera->camera.effects & CameraEffectsFlags::MotionBlur) ? velocityTexture : TextureHandle());
					renderQueue->depthTexture(depthTexture);
					renderQueue->activeAttachments(31);
					renderQueue->checkFrameBuffer();
					renderQueue->checkGlErrorDebug();

					// clear
					renderQueue->viewport(Vec2i(), pass.resolution);
					const auto cf = pass.camera->camera.clear;
					if (any(cf))
						renderQueue->clear(any(cf & CameraClearFlags::Color), any(cf & CameraClearFlags::Depth), any(cf & CameraClearFlags::Stencil));
				}

				TextureHandle texSource = colorTexture;
				TextureHandle texTarget = intermediateTexture;

				ScreenSpaceCommonConfig gfCommonConfig; // helper to simplify initialization
				gfCommonConfig.assets = engineAssets();
				gfCommonConfig.provisionals = +provisionalData;
				gfCommonConfig.queue = +renderQueue;
				gfCommonConfig.resolution = pass.resolution;

				{ // deferred
					const auto graphicsDebugScope = renderQueue->namedScope("deferred pass");

					renderQueue->universalUniformStruct(pass.viewport, CAGE_SHADER_UNIBLOCK_VIEWPORT);

					{ // opaque objects
						const auto graphicsDebugScope = renderQueue->namedScope("opaque objects");
						renderQueue->bind(shaderGBuffer);
						for (const auto &it : pass.opaque.data)
							renderInstances(it.second);
						renderQueue->resetAllState();
						renderQueue->checkGlErrorDebug();
					}

					{ // render target
						const auto graphicsDebugScope = renderQueue->namedScope("prepare render target");
						renderQueue->bind(renderTarget);
						renderQueue->clearFrameBuffer();
						renderQueue->colorTexture(0, colorTexture);
						renderQueue->activeAttachments(1);
						renderQueue->checkFrameBuffer();
						renderQueue->checkGlErrorDebug();
					}

					{ // bind gBuffer textures for sampling
						const auto graphicsDebugScope = renderQueue->namedScope("bind gBuffer textures");
						renderQueue->bind(albedoTexture, CAGE_SHADER_TEXTURE_ALBEDO);
						renderQueue->bind(specialTexture, CAGE_SHADER_TEXTURE_SPECIAL);
						renderQueue->bind(normalTexture, CAGE_SHADER_TEXTURE_NORMAL);
						renderQueue->bind(depthTexture, CAGE_SHADER_TEXTURE_DEPTH);
						renderQueue->bind(colorTexture, CAGE_SHADER_TEXTURE_COLOR);
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

						renderQueue->viewport(Vec2i(), pass.resolution);
						renderQueue->culling(true);
						renderQueue->blending(true);
						renderQueue->blendFuncAdditive();
						renderQueue->bind(shaderLighting);

						renderLightsCollection(pass.lights, bindLightType);

						renderQueue->resetAllState();
						renderQueue->checkGlErrorDebug();
					}
				}

				{ // opaque effects
					const auto graphicsDebugScope2 = renderQueue->namedScope("opaque effects");
					renderQueue->bind(renderTarget);
					renderQueue->clearFrameBuffer();
					renderQueue->activeAttachments(1);

					const CameraEffectsFlags effects = pass.camera->camera.effects;
					const auto &params = pass.camera->camera;

					// ssao
					if (any(effects & CameraEffectsFlags::AmbientOcclusion))
					{
						ScreenSpaceAmbientOcclusionConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
						(ScreenSpaceAmbientOcclusion &)cfg = params.ssao;
						cfg.frameIndex = frameIndex;
						cfg.inDepth = depthTexture;
						cfg.inNormal = normalTexture;
						cfg.outAo = ambientOcclusionTexture;
						cfg.viewProj = pass.viewProj;
						screenSpaceAmbientOcclusion(cfg);
					}

					// ambient light
					if ((pass.viewport.ambientLight + pass.viewport.ambientDirectionalLight) != Vec4())
					{
						const auto graphicsDebugScope = renderQueue->namedScope("ambient light");
						renderQueue->viewport(Vec2i(), pass.resolution);
						renderQueue->colorTexture(0, texTarget);
						renderQueue->bind(shaderAmbient);
						if (any(effects & CameraEffectsFlags::AmbientOcclusion))
						{
							renderQueue->bind(ambientOcclusionTexture, CAGE_SHADER_TEXTURE_EFFECTS);
							renderQueue->uniform(CAGE_SHADER_UNI_AMBIENTOCCLUSION, 1);
						}
						else
							renderQueue->uniform(CAGE_SHADER_UNI_AMBIENTOCCLUSION, 0);
						renderQueue->bind(texSource, CAGE_SHADER_TEXTURE_COLOR);
						renderQueue->bind(modelSquare);
						renderQueue->draw();
						renderQueue->checkGlErrorDebug();
						std::swap(texSource, texTarget);
					}

					// depth of field
					if (any(effects & CameraEffectsFlags::DepthOfField))
					{
						ScreenSpaceDepthOfFieldConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
						(ScreenSpaceDepthOfField &)cfg = params.depthOfField;
						cfg.proj = pass.proj;
						cfg.inDepth = depthTexture;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						screenSpaceDepthOfField(cfg);
						std::swap(texSource, texTarget);
					}

					// motion blur
					if (any(effects & CameraEffectsFlags::MotionBlur))
					{
						ScreenSpaceMotionBlurConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
						(ScreenSpaceMotionBlur &)cfg = params.motionBlur;
						cfg.inVelocity = velocityTexture;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						screenSpaceMotionBlur(cfg);
						std::swap(texSource, texTarget);
					}

					// eye adaptation
					if (any(effects & CameraEffectsFlags::EyeAdaptation))
					{
						ScreenSpaceEyeAdaptationConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
						(ScreenSpaceEyeAdaptation &)cfg = params.eyeAdaptation;
						cfg.cameraId = Stringizer() + pass.camera->emit->entityId;
						cfg.inColor = texSource;
						screenSpaceEyeAdaptationPrepare(cfg);
					}

					if (texSource != colorTexture)
					{
						renderQueue->colorTexture(0, texTarget);
						renderQueue->bind(shaderBlit);
						renderQueue->bind(texSource, CAGE_SHADER_TEXTURE_COLOR);
						renderQueue->draw();
						renderQueue->checkGlErrorDebug();
						std::swap(texSource, texTarget);
					}
					CAGE_ASSERT(texSource == colorTexture);
					CAGE_ASSERT(texTarget == intermediateTexture);

					renderQueue->resetAllState();
				}

				{ // transparencies
					const auto graphicsDebugScope = renderQueue->namedScope("transparencies");

					renderQueue->bind(renderTarget);
					renderQueue->clearFrameBuffer();
					renderQueue->colorTexture(0, colorTexture);
					renderQueue->depthTexture(depthTexture);
					renderQueue->activeAttachments(1);
					renderQueue->checkFrameBuffer();

					renderQueue->viewport(Vec2i(), pass.resolution);
					renderQueue->depthTest(true);
					renderQueue->blending(true);
					renderQueue->bind(shaderTranslucent);

					for (TranslucentsInstances &t : pass.translucents.data)
					{
						CAGE_ASSERT(t.renders.data.size() == 1);

						{ // render ambient object
							renderQueue->blendFuncPremultipliedTransparency();
							uint32 tmp = CAGE_SHADER_ROUTINEPROC_LIGHTFORWARDBASE;
							uint32 &orig = t.renders.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE];
							std::swap(tmp, orig);
							renderInstances(t.renders);
							std::swap(tmp, orig);
						}

						{ // render lights on the object
							renderQueue->blendFuncAdditive();

							const auto &bindLightType = [&](LightTypeEnum type, bool shadows)
							{
								uint32 (&shaderRoutines)[CAGE_SHADER_MAX_ROUTINES] = t.renders.shaderRoutines;
								switch (type)
								{
								case LightTypeEnum::Directional:
									shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE] = shadows ? CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONALSHADOW : CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONAL;
									break;
								case LightTypeEnum::Spot:
									shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE] = shadows ? CAGE_SHADER_ROUTINEPROC_LIGHTSPOTSHADOW : CAGE_SHADER_ROUTINEPROC_LIGHTSPOT;
									break;
								case LightTypeEnum::Point:
									shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE] = shadows ? CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW : CAGE_SHADER_ROUTINEPROC_LIGHTPOINT;
									break;
								default:
									CAGE_THROW_CRITICAL(Exception, "invalid light type");
								}
								renderQueue->uniform(CAGE_SHADER_UNI_ROUTINES, shaderRoutines);
							};

							renderLightsCollection(t.lights, bindLightType);
						}
					}

					renderQueue->resetAllState();
					renderQueue->checkGlErrorDebug();
				}

				{ // texts
					const auto graphicsDebugScope = renderQueue->namedScope("texts");

					renderQueue->viewport(Vec2i(), pass.resolution);
					renderQueue->depthTest(true);
					renderQueue->depthWrite(false);
					renderQueue->culling(true);
					renderQueue->blending(true);
					renderQueue->blendFuncAlphaTransparency();

					for (const auto &t : pass.texts.data)
					{
						t.first->bind(+renderQueue, modelSquare, shaderFont);
						for (const PrepText *r : t.second.data)
						{
							renderQueue->uniform(0, pass.viewProj * r->model);
							renderQueue->uniform(4, r->color);
							t.first->render(+renderQueue, r->glyphs, r->format);
						}
					}

					renderQueue->resetAllState();
					renderQueue->checkGlErrorDebug();
				}

				{ // final effects
					const auto graphicsDebugScope2 = renderQueue->namedScope("final effects");

					const CameraEffectsFlags effects = pass.camera->camera.effects;
					const auto &params = pass.camera->camera;

					// bloom
					if (any(effects & CameraEffectsFlags::Bloom))
					{
						ScreenSpaceBloomConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
						(ScreenSpaceBloom &)cfg = params.bloom;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						screenSpaceBloom(cfg);
						std::swap(texSource, texTarget);
					}

					// eye adaptation
					if (any(effects & CameraEffectsFlags::EyeAdaptation))
					{
						ScreenSpaceEyeAdaptationConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
						(ScreenSpaceEyeAdaptation &)cfg = params.eyeAdaptation;
						cfg.cameraId = Stringizer() + pass.camera->emit->entityId;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						screenSpaceEyeAdaptationApply(cfg);
						std::swap(texSource, texTarget);
					}

					// final screen effects
					if (any(effects & (CameraEffectsFlags::ToneMapping | CameraEffectsFlags::GammaCorrection)))
					{
						ScreenSpaceTonemapConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
						(ScreenSpaceTonemap &)cfg = params.tonemap;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						cfg.tonemapEnabled = any(effects & CameraEffectsFlags::ToneMapping);
						cfg.gamma = any(effects & CameraEffectsFlags::GammaCorrection) ? params.gamma : 1;
						screenSpaceTonemap(cfg);
						std::swap(texSource, texTarget);
					}

					// fxaa
					if (any(effects & CameraEffectsFlags::AntiAliasing))
					{
						ScreenSpaceFastApproximateAntiAliasingConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = gfCommonConfig;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						screenSpaceFastApproximateAntiAliasing(cfg);
						std::swap(texSource, texTarget);
					}

					renderQueue->resetAllState();
				}

				// blit to destination
				if (pass.camera->target || texSource != colorTexture)
				{
					const auto graphicsDebugScope = renderQueue->namedScope("blit to destination");
					renderQueue->bind(texSource, 0);
					renderQueue->bind(renderTarget);
					if (pass.camera->target)
						renderQueue->colorTexture(0, pass.camera->target);
					else
						renderQueue->colorTexture(0, colorTexture);
					renderQueue->activeAttachments(1);
					renderQueue->viewport(Vec2i(), pass.resolution);
					renderQueue->bind(shaderBlit);
					renderQueue->bind(modelSquare);
					renderQueue->draw();
					renderQueue->resetAllState();
				}

				renderQueue->resetAllTextures();
				renderQueue->resetFrameBuffer();
				renderQueue->checkGlErrorDebug();
			}

			void blitToWindow()
			{
				const String suffix = Stringizer() + "_" + cameraPasses.back().resolution[0] + "_" + cameraPasses.back().resolution[1];
				std::vector<std::pair<TextureHandle, Holder<ShaderProgram>>> sources;
				sources.reserve(10);
				sources.emplace_back(provisionalData->texture(Stringizer() + "colorTexture" + suffix), shaderVisualizeColor.share());
				sources.emplace_back(provisionalData->texture(Stringizer() + "albedoTexture" + suffix), shaderVisualizeColor.share());
				sources.emplace_back(provisionalData->texture(Stringizer() + "specialTexture" + suffix), shaderVisualizeColor.share());
				sources.emplace_back(provisionalData->texture(Stringizer() + "normalTexture" + suffix), shaderVisualizeColor.share());
				sources.emplace_back(provisionalData->texture(Stringizer() + "velocityTexture" + suffix), shaderVisualizeVelocity.share());
				sources.emplace_back(provisionalData->texture(Stringizer() + "depthTexture" + suffix), shaderVisualizeDepth.share());
				sources.emplace_back(provisionalData->texture(Stringizer() + "ambientOcclusionTexture" + suffix), shaderVisualizeMonochromatic.share());
				for (const auto &cp : cameraPasses)
					if (cp.camera->target)
						sources.emplace_back(cp.camera->target, shaderVisualizeColor.share());

				const uint32 mod = numeric_cast<uint32>(sources.size());
				sint32 idx = confVisualizeBuffer;
				idx = ((idx % mod) + mod) % mod;
				CAGE_ASSERT(idx >= 0 && idx < mod);

				const auto graphicsDebugScope = renderQueue->namedScope("blit to window");
				renderQueue->bind(sources[idx].first, 0);
				renderQueue->resetFrameBuffer();
				renderQueue->viewport(Vec2i(), windowResolution);
				renderQueue->bind(sources[idx].second);
				renderQueue->uniform(0, 1.0 / Vec2(windowResolution));
				renderQueue->bind(modelSquare);
				renderQueue->draw();
				renderQueue->resetAllState();
				renderQueue->resetAllTextures();
				renderQueue->resetFrameBuffer();
				renderQueue->checkGlErrorDebug();
			}

			void run()
			{
				if (windowResolution[0] <= 0 || windowResolution[1] <= 0)
					return;
				if (!loadGeneralAssets())
					return;

				copyEmitToPrep();
				if (cameras.empty())
					return;

				// sort cameras
				std::sort(cameras.begin(), cameras.end(), [](const PrepCamera &aa, const PrepCamera &bb) {
					const std::pair<bool, sint32> a = { !aa.target, aa.camera.cameraOrder };
					const std::pair<bool, sint32> b = { !bb.target, bb.camera.cameraOrder };
					CAGE_ASSERT(a != b);
					return a < b;
				});

				// initialize passes
				cameraPasses.reserve(cameras.size());
				for (const auto &it : cameras)
				{
					cameraPasses.push_back({});
					CameraPass &c = cameraPasses.back();
					c.camera = &it;
					c.task = tasksRunAsync("initialize camera", Delegate<void(CameraPass &, uint32)>().bind<Preparator, &Preparator::initializeCameraPass>(this), Holder<CameraPass>(&c, nullptr));
				}

				// generate passes
				for (auto &c : cameraPasses)
				{
					c.task->wait();
					for (auto &s : c.shadowmapPasses)
						s->task = tasksRunAsync("generate shadowmap", Delegate<void(ShadowmapPass &, uint32)>().bind<Preparator, &Preparator::generateShadowmapPass>(this), Holder<ShadowmapPass>(+s, nullptr));
					c.task = tasksRunAsync("generate camera", Delegate<void(CameraPass &, uint32)>().bind<Preparator, &Preparator::generateCameraPass>(this), Holder<CameraPass>(&c, nullptr));
				}

				// render passes
				for (auto &c : cameraPasses)
				{
					for (auto &s : c.shadowmapPasses)
					{
						s->task->wait();
						renderShadowmapPass(*s);
					}
					c.task->wait();
					renderCameraPass(c);
				}
				blitToWindow();
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
