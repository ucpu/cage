#include <cage-core/skeletalAnimationPreparator.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/swapBufferGuard.h>
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

#include <algorithm>
#include <optional>
#include <vector>
#include <map>

namespace cage
{
	namespace
	{
		ConfigSint32 confVisualizeBuffer("cage/graphics/visualizeBuffer", 0);
		ConfigBool confRenderMissingModels("cage/graphics/renderMissingModels", false);
		ConfigBool confRenderSkeletonBones("cage/graphics/renderSkeletonBones", false);
		ConfigBool confNoAmbientOcclusion("cage/graphics/disableAmbientOcclusion", false);
		ConfigBool confNoBloom("cage/graphics/disableBloom", false);

		struct UniMesh
		{
			Mat4 mvpMat;
			Mat3x4 normalMat; // [2][3] is 1 if lighting is enabled and 0 otherwise
			Mat3x4 mMat;
			Vec4 color; // color rgb is linear, and NOT alpha-premultiplied
			Vec4 aniTexFrames;
		};

		struct UniLight
		{
			Vec4 color;
			Vec4 position;
			Vec4 direction;
			Vec4 attenuation;
			Vec4 parameters; // spotAngle, spotExponent, normalOffsetScale, lightType
		};

		struct UniViewport
		{
			Mat4 vpInv; // viewProj inverse
			Vec4 eyePos;
			Vec4 eyeDir;
			Vec4 viewport; // x, y, w, h
			Vec4 ambientLight; // color rgb is linear, no alpha
			Vec4 ambientDirectionalLight; // color rgb is linear, no alpha
		};

		struct RenderInfo
		{
			RenderComponent render;
			UniMesh uni;
			Mat4 model;
			Frustum frustum; // object-space camera frustum used for culling
			Entity *e = nullptr;

			void initUniMatrices(const struct CommonData &data);
		};

		struct RenderInfoEx : public RenderInfo
		{
			Holder<SkeletalAnimationPreparatorInstance> skeletalAnimation;
			Holder<TextureAnimationComponent> textureAnimation;
			Holder<Texture> textures[MaxTexturesCountPerMaterial];
			uint32 shaderRoutines[CAGE_SHADER_MAX_ROUTINES] = {};
			bool blending = false;
		};

		struct OpaqueInstanceKey
		{
			Holder<ShaderProgram> shader;
			Holder<Model> model;

			bool operator < (const OpaqueInstanceKey &other) const = default;
		};

		struct Translucent
		{
			Holder<ShaderProgram> shader;
			Holder<Model> model;
			RenderInfoEx info;
		};

		struct DebugVisualizationInfo
		{
			TextureHandle texture;
			Holder<ShaderProgram> shader;
		};

		// camera or light
		struct CommonData
		{
			Mat4 model;
			Mat4 view;
			Mat4 viewProj;
			Mat4 proj;
			Vec2i resolution;
			uint32 sceneMask = 0;
			Entity *entity = nullptr;

			struct LodSelection
			{
				Vec3 center; // center of camera (NOT light)
				Real screenSize = 0; // vertical size of screen in pixels, one meter in front of the camera
				bool orthographic = false;
			} lodSelection;

			//std::map<OpaqueInstanceKey, std::vector<RenderInfoEx>> opaque;
			std::vector<DebugVisualizationInfo> debugVisualizations;
			Holder<RenderQueue> renderQueue = newRenderQueue();
		};

		struct ShadowmapData : public CommonData
		{
			Mat4 shadowMat;
			Holder<ProvisionalTexture> shadowTexture;
			LightComponent lightComponent;
			ShadowmapComponent shadowmapComponent;
			UubRange lighsBlock;
		};

		struct CameraData : public CommonData
		{
			CameraComponent camera;
			//std::vector<Translucent> translucent;
			std::map<Entity *, ShadowmapData> shadowmaps;
			UubRange lighsBlock;
			uint32 lightsCount = 0;
		};

		void RenderInfo::initUniMatrices(const CommonData &data)
		{
			uni.mMat = Mat3x4(model);
			uni.mvpMat = data.viewProj * model;
			uni.normalMat = Mat3x4(inverse(Mat3(model)));
		}

		void initializeLodSelection(CommonData &data, const CameraComponent &cam, const Mat4 &camModel)
		{
			switch (cam.cameraType)
			{
			case CameraTypeEnum::Orthographic:
			{
				data.lodSelection.screenSize = cam.camera.orthographicSize[1] * data.resolution[1];
				data.lodSelection.orthographic = true;
			} break;
			case CameraTypeEnum::Perspective:
				data.lodSelection.screenSize = tan(cam.camera.perspectiveFov * 0.5) * 2 * data.resolution[1];
				break;
			default:
				CAGE_THROW_ERROR(Exception, "invalid camera type");
			}
			data.lodSelection.center = Vec3(camModel * Vec4(0, 0, 0, 1));
		}

		UniLight initializeLightUni(const Mat4 &model, const LightComponent &lc)
		{
			UniLight uni;
			uni.color = Vec4(colorGammaToLinear(lc.color) * lc.intensity, 0);
			uni.position = model * Vec4(0, 0, 0, 1);
			uni.direction = model * Vec4(0, 0, -1, 0);
			uni.attenuation = Vec4(lc.attenuation, 0);
			uni.parameters[0] = cos(lc.spotAngle * 0.5);
			uni.parameters[1] = lc.spotExponent;
			return uni;
		}

		void updateShaderRoutinesForTextures(const Holder<Texture> textures[MaxTexturesCountPerMaterial], uint32 shaderRoutines[CAGE_SHADER_MAX_ROUTINES])
		{
			if (textures[CAGE_SHADER_TEXTURE_ALBEDO])
			{
				switch (textures[CAGE_SHADER_TEXTURE_ALBEDO]->target())
				{
				case GL_TEXTURE_2D_ARRAY:
					shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPALBEDO] = CAGE_SHADER_ROUTINEPROC_MAPALBEDOARRAY;
					break;
				case GL_TEXTURE_CUBE_MAP:
					shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPALBEDO] = CAGE_SHADER_ROUTINEPROC_MAPALBEDOCUBE;
					break;
				default:
					shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPALBEDO] = CAGE_SHADER_ROUTINEPROC_MAPALBEDO2D;
					break;
				}
			}
			else
				shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPALBEDO] = 0;

			if (textures[CAGE_SHADER_TEXTURE_SPECIAL])
			{
				switch (textures[CAGE_SHADER_TEXTURE_SPECIAL]->target())
				{
				case GL_TEXTURE_2D_ARRAY:
					shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL] = CAGE_SHADER_ROUTINEPROC_MAPSPECIALARRAY;
					break;
				case GL_TEXTURE_CUBE_MAP:
					shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL] = CAGE_SHADER_ROUTINEPROC_MAPSPECIALCUBE;
					break;
				default:
					shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL] = CAGE_SHADER_ROUTINEPROC_MAPSPECIAL2D;
					break;
				}
			}
			else
				shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL] = 0;

			if (textures[CAGE_SHADER_TEXTURE_NORMAL])
			{
				switch (textures[CAGE_SHADER_TEXTURE_NORMAL]->target())
				{
				case GL_TEXTURE_2D_ARRAY:
					shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPNORMAL] = CAGE_SHADER_ROUTINEPROC_MAPNORMALARRAY;
					break;
				case GL_TEXTURE_CUBE_MAP:
					shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPNORMAL] = CAGE_SHADER_ROUTINEPROC_MAPNORMALCUBE;
					break;
				default:
					shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPNORMAL] = CAGE_SHADER_ROUTINEPROC_MAPNORMAL2D;
					break;
				}
			}
			else
				shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPNORMAL] = 0;
		}

		enum class RenderModeEnum
		{
			Shadowmap,
			DepthPrepass,
			Standard,
		};

		struct Preparator
		{
			const bool cnfRenderMissingModels = confRenderMissingModels;
			const bool cnfRenderSkeletonBones = confRenderSkeletonBones;
			const sint32 cnfVisualizeBuffer = confVisualizeBuffer;
			const Holder<ProvisionalGraphics> &provisionalData = graphics->provisionalData;
			const Holder<SkeletalAnimationPreparatorCollection> skeletonPreparatorCollection = newSkeletalAnimationPreparatorCollection(engineAssets(), cnfRenderSkeletonBones);
			const Vec2i windowResolution = engineWindow()->resolution();
			const uint32 frameIndex = graphics->frameIndex;
			const EmitBuffer &emit;
			const uint64 prepareTime;
			const Real interpolationFactor;
			EntityComponent *const transformComponent = nullptr;
			EntityComponent *const prevTransformComponent = nullptr;

			Holder<Model> modelSquare, modelBone;
			Holder<ShaderProgram> shaderBlit, shaderDepth, shaderStandard;
			Holder<ShaderProgram> shaderVisualizeColor, shaderVisualizeDepth, shaderVisualizeMonochromatic, shaderVisualizeVelocity;
			Holder<ShaderProgram> shaderFont;

			Preparator(const EmitBuffer &emit, uint64 time)
				: emit(emit),
				prepareTime(graphics->itc(emit.time, time, controlThread().updatePeriod())),
				interpolationFactor(saturate(Real(prepareTime - emit.time) / controlThread().updatePeriod())),
				transformComponent(emit.scene->component<TransformComponent>()),
				prevTransformComponent(emit.scene->componentsByType(detail::typeIndex<TransformComponent>())[1])
			{}

			bool loadGeneralAssets()
			{
				const AssetManager *ass = engineAssets();
				if (!ass->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/cage.pack")) || !ass->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/shader/engine/engine.pack")))
					return false;

				modelSquare = ass->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
				modelBone = ass->get<AssetSchemeIndexModel, Model>(HashString("cage/model/bone.obj"));
				shaderBlit = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/blit.glsl"));
				shaderDepth = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/depth.glsl"));
				shaderStandard = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/engine/standard.glsl"));
				shaderVisualizeColor = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/visualize/color.glsl"));
				shaderVisualizeDepth = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/visualize/depth.glsl"));
				shaderVisualizeMonochromatic = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/visualize/monochromatic.glsl"));
				shaderVisualizeVelocity = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/visualize/velocity.glsl"));
				shaderFont = ass->get<AssetSchemeIndexShaderProgram, ShaderProgram>(HashString("cage/shader/gui/font.glsl"));
				CAGE_ASSERT(shaderBlit);
				return true;
			}

			Mat4 modelTransform(Entity *e) const
			{
				CAGE_ASSERT(e->has(transformComponent));
				if (e->has(prevTransformComponent))
				{
					const Transform c = e->value<TransformComponent>(transformComponent);
					const Transform p = e->value<TransformComponent>(prevTransformComponent);
					return Mat4(interpolate(p, c, interpolationFactor));
				}
				else
					return Mat4(e->value<TransformComponent>(transformComponent));
			}

			template<RenderModeEnum RenderMode>
			void renderModelImpl(const CommonData &data, RenderInfoEx &ex, Holder<Model> model)
			{
				const Holder<RenderQueue> &renderQueue = data.renderQueue;
				renderQueue->bind(model);
				renderQueue->bind(RenderMode == RenderModeEnum::Standard ? shaderStandard : shaderDepth);
				renderQueue->culling(!any(model->flags & MeshRenderFlags::TwoSided));
				renderQueue->depthTest(any(model->flags & MeshRenderFlags::DepthTest));
				renderQueue->depthWrite(any(model->flags & MeshRenderFlags::DepthWrite));
				ex.uni.normalMat.data[2][3] = any(model->flags & MeshRenderFlags::Lighting) ? 1 : 0; // is lighting enabled

				for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
				{
					const uint32 n = model->textureName(i);
					if (!n)
						continue;
					ex.textures[i] = engineAssets()->tryGet<AssetSchemeIndexTexture, Texture>(n);
					switch (ex.textures[i]->target())
					{
					case GL_TEXTURE_2D_ARRAY: renderQueue->bind(ex.textures[i], CAGE_SHADER_TEXTURE_ALBEDO_ARRAY + i); break;
					case GL_TEXTURE_CUBE_MAP: renderQueue->bind(ex.textures[i], CAGE_SHADER_TEXTURE_ALBEDO_CUBE + i);break;
					default: renderQueue->bind(ex.textures[i], CAGE_SHADER_TEXTURE_ALBEDO + i); break;
					}
				}
				if (ex.textureAnimation && ex.textures[CAGE_SHADER_TEXTURE_ALBEDO])
				{
					const auto &p = *ex.textureAnimation;
					ex.uni.aniTexFrames = detail::evalSamplesForTextureAnimation(+ex.textures[CAGE_SHADER_TEXTURE_ALBEDO], prepareTime, p.startTime, p.speed, p.offset);
				}
				updateShaderRoutinesForTextures(ex.textures, ex.shaderRoutines);

				ex.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_SKELETON] = ex.skeletalAnimation ? CAGE_SHADER_ROUTINEPROC_SKELETONANIMATION : 0;
				if (ex.skeletalAnimation)
				{
					const auto armature = ex.skeletalAnimation->armature();
					CAGE_ASSERT(armature.size() == model->skeletonBones);
					renderQueue->uniform(CAGE_SHADER_UNI_BONESPERINSTANCE, model->skeletonBones);
					renderQueue->universalUniformArray(armature, CAGE_SHADER_UNIBLOCK_ARMATURES);
				}

				renderQueue->universalUniformArray<UniMesh>({ &ex.uni, &ex.uni + 1 }, CAGE_SHADER_UNIBLOCK_MESHES);
				renderQueue->uniform(CAGE_SHADER_UNI_ROUTINES, ex.shaderRoutines);

				if constexpr (RenderMode == RenderModeEnum::Standard)
				{
					const CameraData &cam = (const CameraData &)data;
					renderQueue->depthFuncLessEqual();
					renderQueue->blending(ex.blending);
					renderQueue->blendFuncPremultipliedTransparency();
					if (cam.lightsCount > 0)
						renderQueue->bind(cam.lighsBlock, CAGE_SHADER_UNIBLOCK_LIGHTS);
					renderQueue->uniform(CAGE_SHADER_UNI_LIGHTSCOUNT, cam.lightsCount);
					renderQueue->draw();

					renderQueue->depthFuncLessEqual();
					renderQueue->blending(true);
					renderQueue->blendFuncAdditive();
					renderQueue->uniform(CAGE_SHADER_UNI_LIGHTSCOUNT, uint32(1));
					for (const auto &it : cam.shadowmaps)
					{
						const ShadowmapData &sh = it.second;
						renderQueue->bind(sh.shadowTexture, sh.lightComponent.lightType == LightTypeEnum::Point ? CAGE_SHADER_TEXTURE_SHADOW_CUBE : CAGE_SHADER_TEXTURE_SHADOW);
						renderQueue->bind(sh.lighsBlock, CAGE_SHADER_UNIBLOCK_LIGHTS);
						renderQueue->uniform(CAGE_SHADER_UNI_SHADOWMATRIX, sh.shadowMat);
						renderQueue->draw();
					}
				}
				else
				{
					renderQueue->depthFuncLess();
					renderQueue->blending(false);
					renderQueue->draw();
				}

				renderQueue->checkGlErrorDebug();
			}

			template<RenderModeEnum RenderMode>
			void renderModelBones(const CommonData &data, const RenderInfoEx &ex)
			{
				const auto armature = ex.skeletalAnimation->armature();
				for (uint32 i = 0; i < armature.size(); i++)
				{
					const Mat4 am = Mat4(armature[i]);
					RenderInfoEx r;
					(RenderInfo &)r = (const RenderInfo &)ex;
					r.model = ex.model * am;
					r.initUniMatrices(data);
					r.uni.color = Vec4(colorGammaToLinear(colorHsvToRgb(Vec3(Real(i) / Real(armature.size()), 1, 1))), r.render.opacity);
					renderModelImpl<RenderMode>(data, r, modelBone.share());
				}
			}

			void updateRenderInfo(RenderInfoEx &pr, const RenderObject *parent)
			{
				Holder<TextureAnimationComponent> &pt = pr.textureAnimation;
				if (pr.e->has<TextureAnimationComponent>())
					pt = systemMemory().createHolder<TextureAnimationComponent>(pr.e->value<TextureAnimationComponent>());

				std::optional<SkeletalAnimationComponent> ps;
				if (pr.e->has<SkeletalAnimationComponent>())
					ps = pr.e->value<SkeletalAnimationComponent>();

				if (parent)
				{
					if (!pr.render.color.valid())
						pr.render.color = parent->color;
					if (!pr.render.intensity.valid())
						pr.render.intensity = parent->intensity;
					if (!pr.render.opacity.valid())
						pr.render.opacity = parent->opacity;

					if (!pt && (parent->texAnimSpeed.valid() || parent->texAnimOffset.valid()))
						pt = systemMemory().createHolder<TextureAnimationComponent>();
					if (pt)
					{
						if (!pt->speed.valid())
							pt->speed = parent->texAnimSpeed;
						if (!pt->offset.valid())
							pt->offset = parent->texAnimOffset;
					}

					if (!ps && parent->skelAnimName)
						ps = SkeletalAnimationComponent();
					if (ps)
					{
						if (!ps->name)
							ps->name = parent->skelAnimName;
						if (!ps->speed.valid())
							ps->speed = parent->skelAnimSpeed;
						if (!ps->offset.valid())
							ps->offset = parent->skelAnimOffset;
					}
				}

				if (!pr.render.color.valid())
					pr.render.color = Vec3(0);
				if (!pr.render.intensity.valid())
					pr.render.intensity = 1;
				if (!pr.render.opacity.valid())
					pr.render.opacity = 1;
				pr.uni.color = Vec4(colorGammaToLinear(pr.render.color) * pr.render.intensity, pr.render.opacity);

				if (pt)
				{
					if (!pt->speed.valid())
						pt->speed = 1;
					if (!pt->offset.valid())
						pt->offset = 0;
				}

				if (ps)
				{
					if (ps->name)
					{
						if (!ps->speed.valid())
							ps->speed = 1;
						if (!ps->offset.valid())
							ps->offset = 0;
					}
					else
						ps.reset();
				}

				if (ps)
				{
					Holder<SkeletalAnimation> anim = engineAssets()->tryGet<AssetSchemeIndexSkeletalAnimation, SkeletalAnimation>(ps->name);
					if (anim)
					{
						Real coefficient = detail::evalCoefficientForSkeletalAnimation(+anim, prepareTime, ps->startTime, ps->speed, ps->offset);
						pr.skeletalAnimation = skeletonPreparatorCollection->create(pr.e, std::move(anim), coefficient);
					}
				}
			}

			template<RenderModeEnum RenderMode>
			void renderModel(const CommonData &data, const RenderInfo &render, Holder<Model> model, Holder<RenderObject> parent = {})
			{
				if constexpr (RenderMode == RenderModeEnum::Shadowmap)
				{
					if (none(model->flags & MeshRenderFlags::ShadowCast))
						return;
				}

				if (!intersects(model->boundingBox(), render.frustum))
					return;

				RenderInfoEx ex;
				(RenderInfo &)ex = render;
				updateRenderInfo(ex, +parent);

				ex.blending = any(model->flags & MeshRenderFlags::Translucent) || ex.render.opacity < 1;
				if constexpr (RenderMode == RenderModeEnum::DepthPrepass)
				{
					if (ex.blending)
						return;
				}

				if (cnfRenderSkeletonBones && ex.skeletalAnimation)
					renderModelBones<RenderMode>(data, ex);
				else
					renderModelImpl<RenderMode>(data, ex, std::move(model));
			}

			template<RenderModeEnum RenderMode>
			void renderObject(const CommonData &data, const RenderInfo &render, Holder<RenderObject> object)
			{
				CAGE_ASSERT(object->lodsCount() > 0);
				uint32 lod = 0;
				if (object->lodsCount() > 1)
				{
					Real d = 1;
					if (!data.lodSelection.orthographic)
					{
						const Vec4 ep4 = render.model * Vec4(0, 0, 0, 1);
						CAGE_ASSERT(abs(ep4[3] - 1) < 1e-4);
						d = distance(Vec3(ep4), data.lodSelection.center);
					}
					const Real f = data.lodSelection.screenSize * object->worldSize / (d * object->pixelsSize);
					lod = object->lodSelect(f);
				}
				for (uint32 it : object->items(lod))
					renderObjectOrModel<RenderMode>(data, render, it, object.share());
			}

			template<RenderModeEnum RenderMode>
			void renderObjectOrModel(const CommonData &data, const RenderInfo &render, const uint32 name, Holder<RenderObject> parent = {})
			{
				Holder<RenderObject> obj = engineAssets()->tryGet<AssetSchemeIndexRenderObject, RenderObject>(name);
				if (obj)
				{
					renderObject<RenderMode>(data, render, std::move(obj));
					return;
				}
				Holder<Model> md = engineAssets()->tryGet<AssetSchemeIndexModel, Model>(name);
				if (md)
				{
					renderModel<RenderMode>(data, render, std::move(md), std::move(parent));
					return;
				}
				if (cnfRenderMissingModels)
				{
					Holder<Model> fake = engineAssets()->tryGet<AssetSchemeIndexModel, Model>(HashString("cage/model/fake.obj"));
					renderModel<RenderMode>(data, render, std::move(fake), std::move(parent));
					return;
				}
			}

			template<RenderModeEnum RenderMode>
			void renderEntities(const CommonData &data)
			{
				entitiesVisitor([&](Entity *e, const RenderComponent &rc) {
					if ((rc.sceneMask & data.sceneMask) == 0)
						return;
					RenderInfo render;
					render.e = e;
					render.model = modelTransform(e);
					render.render = rc;
					render.initUniMatrices(data);
					render.frustum = Frustum(render.uni.mvpMat);
					renderObjectOrModel<RenderMode>(data, render, rc.object);
				}, +emit.scene, false);
				data.renderQueue->checkGlErrorDebug();
			}

			void renderShadowmap(ShadowmapData &data, uint32)
			{
				Holder<RenderQueue> &renderQueue = data.renderQueue;
				const auto graphicsDebugScope = renderQueue->namedScope("shadowmap");

				renderQueue->bind(data.shadowTexture, CAGE_SHADER_TEXTURE_DEPTH);
				if (data.lightComponent.lightType == LightTypeEnum::Point)
					renderQueue->imageCube(data.resolution, GL_DEPTH_COMPONENT16);
				else
					renderQueue->image2d(data.resolution, GL_DEPTH_COMPONENT24);
				renderQueue->filters(GL_LINEAR, GL_LINEAR, 16);
				renderQueue->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
				renderQueue->checkGlErrorDebug();

				FrameBufferHandle renderTarget = provisionalData->frameBufferDraw("renderTarget");
				renderQueue->bind(renderTarget);
				renderQueue->clearFrameBuffer();
				renderQueue->depthTexture(data.shadowTexture);
				renderQueue->activeAttachments(0);
				renderQueue->checkFrameBuffer();
				renderQueue->viewport(Vec2i(), data.resolution);
				renderQueue->colorWrite(false);
				renderQueue->clear(false, true);
				renderQueue->bind(shaderDepth);
				renderQueue->checkGlErrorDebug();

				renderEntities<RenderModeEnum::Shadowmap>(data);

				renderQueue->resetFrameBuffer();
				renderQueue->resetAllState();
				renderQueue->resetAllTextures();
				renderQueue->checkGlErrorDebug();

				DebugVisualizationInfo deb;
				deb.texture = data.shadowTexture.share();
				deb.shader = shaderVisualizeDepth.share();
				data.debugVisualizations.push_back(std::move(deb));
			}

			void renderCameraLights(CameraData &data)
			{
				std::vector<UniLight> lights;
				lights.reserve(50);
				entitiesVisitor([&](Entity *e, const LightComponent &lc) {
					if ((lc.sceneMask & data.sceneMask) == 0)
						return;
					if (e->has<ShadowmapComponent>())
						return;
					UniLight uni = initializeLightUni(modelTransform(e), lc);
					uni.parameters[3] = [&]() {
						switch (lc.lightType)
						{
						case LightTypeEnum::Directional: return CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONAL;
						case LightTypeEnum::Spot: return CAGE_SHADER_ROUTINEPROC_LIGHTSPOT;
						case LightTypeEnum::Point: return CAGE_SHADER_ROUTINEPROC_LIGHTPOINT;
						default: CAGE_THROW_CRITICAL(Exception, "invalid light type");
						}
					}();
					lights.push_back(uni);
				}, +emit.scene, false);
				data.lightsCount = numeric_cast<uint32>(lights.size());
				if (data.lightsCount > 0)
					data.lighsBlock = data.renderQueue->universalUniformArray<UniLight>(lights);
			}

			void renderCameraEffects(const TextureHandle texSource_, const TextureHandle depthTexture, CameraData &data)
			{
				const detail::StringBase<16> cameraName = Stringizer() + data.entity;
				Holder<RenderQueue> &renderQueue = data.renderQueue;
				const auto graphicsDebugScope = renderQueue->namedScope("effects");

				TextureHandle texSource = texSource_;
				TextureHandle texTarget = [&]() {
					TextureHandle t = provisionalData->texture(Stringizer() + "intermediateTarget_" + data.resolution);
					renderQueue->bind(t, 0);
					renderQueue->filters(GL_LINEAR, GL_LINEAR, 0);
					renderQueue->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					renderQueue->image2d(data.resolution, GL_RGB16F);
					return t;
				}();

				ScreenSpaceCommonConfig commonConfig; // helper to simplify initialization
				commonConfig.assets = engineAssets();
				commonConfig.provisionals = +provisionalData;
				commonConfig.queue = +renderQueue;
				commonConfig.resolution = data.resolution;

				// depth of field
				if (any(data.camera.effects & CameraEffectsFlags::DepthOfField))
				{
					ScreenSpaceDepthOfFieldConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					(ScreenSpaceDepthOfField &)cfg = data.camera.depthOfField;
					cfg.proj = data.proj;
					cfg.inDepth = depthTexture;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					screenSpaceDepthOfField(cfg);
					std::swap(texSource, texTarget);
				}

				// eye adaptation
				if (any(data.camera.effects & CameraEffectsFlags::EyeAdaptation))
				{
					ScreenSpaceEyeAdaptationConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					(ScreenSpaceEyeAdaptation &)cfg = data.camera.eyeAdaptation;
					cfg.cameraId = cameraName;
					cfg.inColor = texSource;
					screenSpaceEyeAdaptationPrepare(cfg);
				}

				// bloom
				if (any(data.camera.effects & CameraEffectsFlags::Bloom))
				{
					ScreenSpaceBloomConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					(ScreenSpaceBloom &)cfg = data.camera.bloom;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					screenSpaceBloom(cfg);
					std::swap(texSource, texTarget);
				}

				// eye adaptation
				if (any(data.camera.effects & CameraEffectsFlags::EyeAdaptation))
				{
					ScreenSpaceEyeAdaptationConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					(ScreenSpaceEyeAdaptation &)cfg = data.camera.eyeAdaptation;
					cfg.cameraId = cameraName;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					screenSpaceEyeAdaptationApply(cfg);
					std::swap(texSource, texTarget);
				}

				// final screen effects
				if (any(data.camera.effects & (CameraEffectsFlags::ToneMapping | CameraEffectsFlags::GammaCorrection)))
				{
					ScreenSpaceTonemapConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					(ScreenSpaceTonemap &)cfg = data.camera.tonemap;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					cfg.tonemapEnabled = any(data.camera.effects & CameraEffectsFlags::ToneMapping);
					cfg.gamma = any(data.camera.effects & CameraEffectsFlags::GammaCorrection) ? data.camera.gamma : 1;
					screenSpaceTonemap(cfg);
					std::swap(texSource, texTarget);
				}

				// fxaa
				if (any(data.camera.effects & CameraEffectsFlags::AntiAliasing))
				{
					ScreenSpaceFastApproximateAntiAliasingConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					screenSpaceFastApproximateAntiAliasing(cfg);
					std::swap(texSource, texTarget);
				}

				// blit to destination
				if (texSource != texSource_)
				{
					renderQueue->viewport(Vec2i(), data.resolution);
					renderQueue->bind(modelSquare);
					renderQueue->bind(texSource, 0);
					renderQueue->bind(shaderBlit);
					renderQueue->bind(provisionalData->frameBufferDraw("renderTarget"));
					renderQueue->colorTexture(0, texSource_);
					renderQueue->activeAttachments(1);
					renderQueue->draw();
				}

				renderQueue->resetFrameBuffer();
				renderQueue->resetAllState();
				renderQueue->resetAllTextures();
				renderQueue->checkGlErrorDebug();
			}

			void renderCamera(CameraData &data, uint32)
			{
				Holder<RenderQueue> &renderQueue = data.renderQueue;
				const auto graphicsDebugScope = renderQueue->namedScope("camera");

				data.sceneMask = data.camera.sceneMask;
				data.resolution = data.camera.target ? data.camera.target->resolution() : windowResolution;
				data.model = modelTransform(data.entity);
				data.view = inverse(data.model);
				data.proj = [&]() {
					switch (data.camera.cameraType)
					{
					case CameraTypeEnum::Orthographic:
					{
						const Vec2 &os = data.camera.camera.orthographicSize;
						return orthographicProjection(-os[0], os[0], -os[1], os[1], data.camera.near, data.camera.far);
					}
					case CameraTypeEnum::Perspective: return perspectiveProjection(data.camera.camera.perspectiveFov, Real(data.resolution[0]) / Real(data.resolution[1]), data.camera.near, data.camera.far);
					default: CAGE_THROW_ERROR(Exception, "invalid camera type");
					}
				}();
				data.viewProj = data.proj * data.view;
				initializeLodSelection(data, data.camera, data.model);

				renderCameraLights(data);

				const String texturesName = data.camera.target ? (Stringizer() + data.entity->name()) : (Stringizer() + data.resolution);
				TextureHandle colorTexture = [&]() {
					TextureHandle t = provisionalData->texture(Stringizer() + "colorTarget_" + texturesName);
					renderQueue->bind(t, 0);
					renderQueue->filters(GL_LINEAR, GL_LINEAR, 0);
					renderQueue->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					renderQueue->image2d(data.resolution, GL_RGB16F);
					DebugVisualizationInfo deb;
					deb.texture = t;
					deb.shader = shaderVisualizeColor.share();
					data.debugVisualizations.push_back(std::move(deb));
					return t;
				}();
				TextureHandle depthTexture = [&]() {
					TextureHandle t = provisionalData->texture(Stringizer() + "depthTarget_" + texturesName);
					renderQueue->bind(t, 0);
					renderQueue->filters(GL_LINEAR, GL_LINEAR, 0);
					renderQueue->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					renderQueue->image2d(data.resolution, GL_DEPTH_COMPONENT32);
					DebugVisualizationInfo deb;
					deb.texture = t;
					deb.shader = shaderVisualizeDepth.share();
					data.debugVisualizations.push_back(std::move(deb));
					return t;
				}();
				FrameBufferHandle renderTarget = provisionalData->frameBufferDraw("renderTarget");
				renderQueue->bind(renderTarget);
				renderQueue->colorTexture(0, colorTexture);
				renderQueue->depthTexture(depthTexture);
				renderQueue->activeAttachments(1);
				renderQueue->checkFrameBuffer();
				renderQueue->checkGlErrorDebug();

				renderQueue->viewport(Vec2i(), data.resolution);
				{
					UniViewport viewport;
					viewport.vpInv = inverse(data.viewProj);
					viewport.eyePos = data.model * Vec4(0, 0, 0, 1);
					viewport.eyeDir = data.model * Vec4(0, 0, -1, 0);
					viewport.ambientLight = Vec4(colorGammaToLinear(data.camera.ambientColor) * data.camera.ambientIntensity, 0);
					viewport.ambientDirectionalLight = Vec4(colorGammaToLinear(data.camera.ambientDirectionalColor) * data.camera.ambientDirectionalIntensity, 0);
					viewport.viewport = Vec4(Vec2(), Vec2(data.resolution));
					renderQueue->universalUniformStruct(viewport, CAGE_SHADER_UNIBLOCK_VIEWPORT);
				}
				if (any(data.camera.clear))
					renderQueue->clear(any(data.camera.clear & CameraClearFlags::Color), any(data.camera.clear & CameraClearFlags::Depth), any(data.camera.clear & CameraClearFlags::Stencil));

				{
					const auto graphicsDebugScope = renderQueue->namedScope("depth prepass");
					renderEntities<RenderModeEnum::DepthPrepass>(data);
				}
				{
					const auto graphicsDebugScope = renderQueue->namedScope("standard");
					renderEntities<RenderModeEnum::Standard>(data);
				}

				renderQueue->resetAllState();
				renderQueue->resetAllTextures();
				renderCameraEffects(colorTexture, depthTexture, data);

				{
					const auto graphicsDebugScope = renderQueue->namedScope("final blit");
					renderQueue->resetAllState();
					renderQueue->viewport(Vec2i(), data.resolution);
					renderQueue->bind(modelSquare);
					renderQueue->bind(colorTexture, 0);
					renderQueue->bind(shaderBlit);
					if (data.camera.target)
					{ // blit to texture
						renderQueue->bind(renderTarget);
						renderQueue->colorTexture(0, Holder<Texture>(data.camera.target, nullptr));
						renderQueue->depthTexture({});
						renderQueue->activeAttachments(1);
						renderQueue->checkFrameBuffer();
						renderQueue->draw();
						renderQueue->resetFrameBuffer();
					}
					else
					{ // blit to window
						renderQueue->resetFrameBuffer();
						renderQueue->draw();
					}
					renderQueue->resetAllState();
					renderQueue->resetAllTextures();
					renderQueue->checkGlErrorDebug();
				}
			}

			Holder<AsyncTask> prepareShadowmap(CameraData &camera, ShadowmapData &data)
			{
				data.sceneMask = data.lightComponent.sceneMask;
				data.resolution = Vec2i(data.shadowmapComponent.resolution);
				data.model = modelTransform(data.entity);
				data.view = inverse(data.model);
				data.proj = [&]() {
					const auto &sc = data.shadowmapComponent;
					switch (data.lightComponent.lightType)
					{
					case LightTypeEnum::Directional: return orthographicProjection(-sc.worldSize[0], sc.worldSize[0], -sc.worldSize[1], sc.worldSize[1], -sc.worldSize[2], sc.worldSize[2]);
					case LightTypeEnum::Spot: return perspectiveProjection(data.lightComponent.spotAngle, 1, sc.worldSize[0], sc.worldSize[1]);
					case LightTypeEnum::Point: return perspectiveProjection(Degs(90), 1, sc.worldSize[0], sc.worldSize[1]);
					default: CAGE_THROW_CRITICAL(Exception, "invalid light type");
					}
				}();
				data.viewProj = data.proj * data.view;
				initializeLodSelection(data, camera.camera, modelTransform(camera.entity));
				constexpr Mat4 bias = Mat4(
					0.5, 0.0, 0.0, 0.0,
					0.0, 0.5, 0.0, 0.0,
					0.0, 0.0, 0.5, 0.0,
					0.5, 0.5, 0.5, 1.0);
				data.shadowMat = bias * data.viewProj;

				const String texName = Stringizer() + "shadowmap_" + data.entity->name() + "_" + camera.entity->name(); // should use stable pointer instead
				if (data.lightComponent.lightType == LightTypeEnum::Point)
					data.shadowTexture = provisionalData->textureCube(texName);
				else
					data.shadowTexture = provisionalData->texture(texName);

				{
					UniLight uni = initializeLightUni(data.model, data.lightComponent);
					uni.parameters[2] = data.entity->value<ShadowmapComponent>().normalOffsetScale;
					uni.parameters[3] = [&]() {
						switch (data.lightComponent.lightType)
						{
						case LightTypeEnum::Directional: return CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONALSHADOW;
						case LightTypeEnum::Spot: return CAGE_SHADER_ROUTINEPROC_LIGHTSPOTSHADOW;
						case LightTypeEnum::Point: return CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW;
						default: CAGE_THROW_CRITICAL(Exception, "invalid light type");
						}
					}();
					// the UubRange is used when rendering with the CAMERA render queue, even if it is stored in the shadowmap data
					data.lighsBlock = camera.renderQueue->universalUniformStruct<UniLight>(uni);
				}

				return tasksRunAsync<ShadowmapData>("render shadowmap task", Delegate<void(ShadowmapData&, uint32)>().bind<Preparator, &Preparator::renderShadowmap>(this), Holder<ShadowmapData>(&data, nullptr));
			}

			void prepareCamera(CameraData &data)
			{
				std::vector<Holder<AsyncTask>> tasks;
				entitiesVisitor([&](Entity *e, const LightComponent &lc, const ShadowmapComponent &sc) {
					if ((lc.sceneMask & data.camera.sceneMask) == 0)
						return;
					ShadowmapData &shd = data.shadowmaps[e];
					shd.entity = e;
					shd.lightComponent = lc;
					shd.shadowmapComponent = sc;
					tasks.push_back(prepareShadowmap(data, shd));
				}, +emit.scene, false);
				tasks.push_back(tasksRunAsync<CameraData>("render camera task", Delegate<void(CameraData&, uint32)>().bind<Preparator, &Preparator::renderCamera>(this), Holder<CameraData>(&data, nullptr)));
				for (auto &it : tasks)
					it->wait();
			}

			void run()
			{
				if (windowResolution[0] <= 0 || windowResolution[1] <= 0)
					return;
				if (!loadGeneralAssets())
					return;

				std::vector<CameraData> cameras;
				entitiesVisitor([&](Entity *e, const CameraComponent &cam) {
					CameraData data;
					data.camera = cam;
					data.entity = e;
					cameras.push_back(std::move(data));
				}, +emit.scene, false);
				std::sort(cameras.begin(), cameras.end(), [](const CameraData &a, const CameraData &b) {
					return std::make_pair(!a.camera.target, a.camera.cameraOrder) < std::make_pair(!b.camera.target, b.camera.cameraOrder);
				});
				tasksRunBlocking<CameraData>("prepare camera task", Delegate<void(CameraData &)>().bind<Preparator, &Preparator::prepareCamera>(this), cameras);

				Holder<RenderQueue> &renderQueue = graphics->renderQueue;
				std::vector<DebugVisualizationInfo> debugVisualizations;
				for (CameraData &cam : cameras)
				{
					for (auto &shm : cam.shadowmaps)
					{
						renderQueue->enqueue(std::move(shm.second.renderQueue));
						for (DebugVisualizationInfo &di : shm.second.debugVisualizations)
							debugVisualizations.push_back(std::move(di));
					}
					renderQueue->enqueue(std::move(cam.renderQueue));
					for (DebugVisualizationInfo &di : cam.debugVisualizations)
						debugVisualizations.push_back(std::move(di));
				}

				const uint32 cnt = numeric_cast<uint32>(debugVisualizations.size()) + 1;
				const uint32 index = (cnfVisualizeBuffer % cnt + cnt) % cnt - 1;
				if (index != m)
				{
					CAGE_ASSERT(index < debugVisualizations.size());
					const auto graphicsDebugScope = renderQueue->namedScope("visualize buffer");
					renderQueue->viewport(Vec2i(), windowResolution);
					renderQueue->bind(modelSquare);
					renderQueue->bind(debugVisualizations[index].texture, 0);
					renderQueue->bind(debugVisualizations[index].shader);
					renderQueue->uniform(0, 1.0 / Vec2(windowResolution));
					renderQueue->draw();
					renderQueue->resetAllState();
					renderQueue->resetAllTextures();
					renderQueue->checkGlErrorDebug();
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
