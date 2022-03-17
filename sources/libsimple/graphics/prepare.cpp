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

			void initUniMatrices(const struct PassInfo &pass);
		};

		struct RenderInfoEx : public RenderInfo
		{
			Holder<SkeletalAnimationPreparatorInstance> skeletalAnimation;
			Holder<TextureAnimationComponent> textureAnimation;
			Holder<Texture> textures[MaxTexturesCountPerMaterial];
			uint32 shaderRoutines[CAGE_SHADER_MAX_ROUTINES] = {};
			bool blending = false;
		};

		// camera or light
		struct PassInfo
		{
			Mat4 model;
			Mat4 view;
			Mat4 viewProj;
			Mat4 proj;
			Vec2i resolution;
			uint32 sceneMask = 0;

			struct LodSelection
			{
				Vec3 center; // center of camera (NOT light)
				Real screenSize = 0; // vertical size of screen in pixels, one meter in front of the camera
				bool orthographic = false;
			} lodSelection;
		};

		struct ShadowmapInfo
		{
			Mat4 shadowMat;
			Holder<ProvisionalTexture> shadowTexture;
		};

		struct DebugVisualizationInfo
		{
			TextureHandle texture;
			Holder<ShaderProgram> shader;
		};

		void RenderInfo::initUniMatrices(const struct PassInfo &pass)
		{
			uni.mMat = Mat3x4(model);
			uni.mvpMat = pass.viewProj * model;
			uni.normalMat = Mat3x4(inverse(Mat3(model)));
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
			const Holder<RenderQueue> &renderQueue = graphics->renderQueue;
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

			std::map<Entity *, ShadowmapInfo> shadowmaps;
			std::vector<DebugVisualizationInfo> debugVisualizations;

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

			template<RenderModeEnum RenderMode>
			void renderModelImpl(const PassInfo &pass, RenderInfoEx &ex, Holder<Model> model)
			{
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
					case GL_TEXTURE_2D_ARRAY:
						renderQueue->bind(ex.textures[i], CAGE_SHADER_TEXTURE_ALBEDO_ARRAY + i);
						break;
					case GL_TEXTURE_CUBE_MAP:
						renderQueue->bind(ex.textures[i], CAGE_SHADER_TEXTURE_ALBEDO_CUBE + i);
						break;
					default:
						renderQueue->bind(ex.textures[i], CAGE_SHADER_TEXTURE_ALBEDO + i);
						break;
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
					std::vector<std::pair<UniLight, const ShadowmapInfo *>> shadowed;
					std::vector<UniLight> lights;
					lights.reserve(50);
					entitiesVisitor([&](Entity *e, const LightComponent &lc) {
						if ((lc.sceneMask & pass.sceneMask) == 0)
							return;
						const bool shadows = e->has<ShadowmapComponent>();
						UniLight uni;
						uni.color = Vec4(colorGammaToLinear(lc.color) * lc.intensity, 0);
						const auto model = modelTransform(e);
						uni.position = model * Vec4(0, 0, 0, 1);
						uni.direction = model * Vec4(0, 0, -1, 0);
						uni.attenuation = Vec4(lc.attenuation, 0);
						uni.parameters[0] = cos(lc.spotAngle * 0.5);
						uni.parameters[1] = lc.spotExponent;
						uni.parameters[2] = shadows ? e->value<ShadowmapComponent>().normalOffsetScale : 0;
						uni.parameters[3] = [&]() {
							switch (lc.lightType)
							{
							case LightTypeEnum::Directional: return shadows ? CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONALSHADOW : CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONAL;
							case LightTypeEnum::Spot: return shadows ? CAGE_SHADER_ROUTINEPROC_LIGHTSPOTSHADOW : CAGE_SHADER_ROUTINEPROC_LIGHTSPOT;
							case LightTypeEnum::Point: return shadows ? CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW : CAGE_SHADER_ROUTINEPROC_LIGHTPOINT;
							default: CAGE_THROW_CRITICAL(Exception, "invalid light type");
							}
						}();
						if (shadows)
							shadowed.emplace_back(uni, &shadowmaps.at(e));
						else
							lights.push_back(uni);
					}, +emit.scene, false);

					renderQueue->depthFuncLessEqual();
					renderQueue->blending(ex.blending);
					renderQueue->blendFuncPremultipliedTransparency();
					if (!lights.empty())
						renderQueue->universalUniformArray<UniLight>(lights, CAGE_SHADER_UNIBLOCK_LIGHTS);
					renderQueue->uniform(CAGE_SHADER_UNI_LIGHTSCOUNT, numeric_cast<uint32>(lights.size()));
					renderQueue->draw();

					renderQueue->depthFuncLessEqual();
					renderQueue->blending(true);
					renderQueue->blendFuncAdditive();
					renderQueue->uniform(CAGE_SHADER_UNI_LIGHTSCOUNT, uint32(1));
					for (const auto &it : shadowed)
					{
						renderQueue->bind(it.second->shadowTexture, it.first.parameters[3] == CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW ? CAGE_SHADER_TEXTURE_SHADOW_CUBE : CAGE_SHADER_TEXTURE_SHADOW);
						renderQueue->universalUniformArray<UniLight>({ &it.first, &it.first + 1 }, CAGE_SHADER_UNIBLOCK_LIGHTS);
						renderQueue->uniform(CAGE_SHADER_UNI_SHADOWMATRIX, it.second->shadowMat);
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
			void renderModelBones(const PassInfo &pass, const RenderInfoEx &ex)
			{
				const auto armature = ex.skeletalAnimation->armature();
				for (uint32 i = 0; i < armature.size(); i++)
				{
					const Mat4 am = Mat4(armature[i]);
					RenderInfoEx r;
					(RenderInfo &)r = (const RenderInfo &)ex;
					r.model = ex.model * am;
					r.initUniMatrices(pass);
					r.uni.color = Vec4(colorGammaToLinear(colorHsvToRgb(Vec3(Real(i) / Real(armature.size()), 1, 1))), r.render.opacity);
					renderModelImpl<RenderMode>(pass, r, modelBone.share());
				}
			}

			static void updateShaderRoutinesForTextures(const Holder<Texture> textures[MaxTexturesCountPerMaterial], uint32 shaderRoutines[CAGE_SHADER_MAX_ROUTINES])
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
			void renderModel(const PassInfo &pass, const RenderInfo &render, Holder<Model> model, Holder<RenderObject> parent = {})
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
					renderModelBones<RenderMode>(pass, ex);
				else
					renderModelImpl<RenderMode>(pass, ex, std::move(model));
			}

			template<RenderModeEnum RenderMode>
			void renderObject(const PassInfo &pass, const RenderInfo &render, Holder<RenderObject> object)
			{
				CAGE_ASSERT(object->lodsCount() > 0);
				uint32 lod = 0;
				if (object->lodsCount() > 1)
				{
					Real d = 1;
					if (!pass.lodSelection.orthographic)
					{
						const Vec4 ep4 = render.model * Vec4(0, 0, 0, 1);
						CAGE_ASSERT(abs(ep4[3] - 1) < 1e-4);
						d = distance(Vec3(ep4), pass.lodSelection.center);
					}
					const Real f = pass.lodSelection.screenSize * object->worldSize / (d * object->pixelsSize);
					lod = object->lodSelect(f);
				}
				for (uint32 it : object->items(lod))
					renderObjectOrModel<RenderMode>(pass, render, it, object.share());
			}

			template<RenderModeEnum RenderMode>
			void renderObjectOrModel(const PassInfo &pass, const RenderInfo &render, const uint32 name, Holder<RenderObject> parent = {})
			{
				Holder<RenderObject> obj = engineAssets()->tryGet<AssetSchemeIndexRenderObject, RenderObject>(name);
				if (obj)
				{
					renderObject<RenderMode>(pass, render, std::move(obj));
					return;
				}
				Holder<Model> md = engineAssets()->tryGet<AssetSchemeIndexModel, Model>(name);
				if (md)
				{
					renderModel<RenderMode>(pass, render, std::move(md), std::move(parent));
					return;
				}
				if (cnfRenderMissingModels)
				{
					Holder<Model> fake = engineAssets()->tryGet<AssetSchemeIndexModel, Model>(HashString("cage/model/fake.obj"));
					renderModel<RenderMode>(pass, render, std::move(fake), std::move(parent));
					return;
				}
			}

			template<RenderModeEnum RenderMode>
			void renderEntities(const PassInfo &pass)
			{
				entitiesVisitor([&](Entity *e, const RenderComponent &rc) {
					if ((rc.sceneMask & pass.sceneMask) == 0)
						return;
					RenderInfo render;
					render.e = e;
					render.model = modelTransform(e);
					render.render = rc;
					render.initUniMatrices(pass);
					render.frustum = Frustum(render.uni.mvpMat);
					renderObjectOrModel<RenderMode>(pass, render, rc.object);
				}, +emit.scene, false);
				renderQueue->checkGlErrorDebug();
			}

			static void initializeLodSelection(PassInfo &pass, const CameraComponent &cam, const Mat4 &camModel)
			{
				switch (cam.cameraType)
				{
				case CameraTypeEnum::Orthographic:
				{
					pass.lodSelection.screenSize = cam.camera.orthographicSize[1] * pass.resolution[1];
					pass.lodSelection.orthographic = true;
				} break;
				case CameraTypeEnum::Perspective:
					pass.lodSelection.screenSize = tan(cam.camera.perspectiveFov * 0.5) * 2 * pass.resolution[1];
					break;
				default:
					CAGE_THROW_ERROR(Exception, "invalid camera type");
				}
				pass.lodSelection.center = Vec3(camModel * Vec4(0, 0, 0, 1));
			}

			Mat4 modelTransform(Entity *e)
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

			void renderShadowmap(Entity *ce, const CameraComponent &cam, Entity *le, const LightComponent &lc, const ShadowmapComponent &sc)
			{
				const auto graphicsDebugScope = renderQueue->namedScope("shadowmap");

				PassInfo pass;
				pass.sceneMask = lc.sceneMask;
				pass.resolution = Vec2i(sc.resolution);
				pass.model = modelTransform(le);
				pass.view = inverse(pass.model);
				pass.proj = [&]() {
					switch (lc.lightType)
					{
					case LightTypeEnum::Directional: return orthographicProjection(-sc.worldSize[0], sc.worldSize[0], -sc.worldSize[1], sc.worldSize[1], -sc.worldSize[2], sc.worldSize[2]);
					case LightTypeEnum::Spot: return perspectiveProjection(lc.spotAngle, 1, sc.worldSize[0], sc.worldSize[1]);
					case LightTypeEnum::Point: return perspectiveProjection(Degs(90), 1, sc.worldSize[0], sc.worldSize[1]);
					default: CAGE_THROW_CRITICAL(Exception, "invalid light type");
					}
				}();
				pass.viewProj = pass.proj * pass.view;
				initializeLodSelection(pass, cam, modelTransform(ce));

				ShadowmapInfo shadowmap;

				constexpr Mat4 bias = Mat4(
					0.5, 0.0, 0.0, 0.0,
					0.0, 0.5, 0.0, 0.0,
					0.0, 0.0, 0.5, 0.0,
					0.5, 0.5, 0.5, 1.0);
				shadowmap.shadowMat = bias * pass.viewProj;

				const String texName = Stringizer() + "shadowmap_" + le->name() + "_" + ce->name(); // should use stable pointer instead
				if (lc.lightType == LightTypeEnum::Point)
				{
					shadowmap.shadowTexture = provisionalData->textureCube(texName);
					renderQueue->bind(shadowmap.shadowTexture, CAGE_SHADER_TEXTURE_DEPTH);
					renderQueue->imageCube(pass.resolution, GL_DEPTH_COMPONENT16);
				}
				else
				{
					shadowmap.shadowTexture = provisionalData->texture(texName);
					renderQueue->bind(shadowmap.shadowTexture, CAGE_SHADER_TEXTURE_DEPTH);
					renderQueue->image2d(pass.resolution, GL_DEPTH_COMPONENT24);
				}
				renderQueue->filters(GL_LINEAR, GL_LINEAR, 16);
				renderQueue->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
				renderQueue->checkGlErrorDebug();

				FrameBufferHandle renderTarget = provisionalData->frameBufferDraw("renderTarget");
				renderQueue->bind(renderTarget);
				renderQueue->clearFrameBuffer();
				renderQueue->depthTexture(shadowmap.shadowTexture);
				renderQueue->activeAttachments(0);
				renderQueue->checkFrameBuffer();
				renderQueue->viewport(Vec2i(), pass.resolution);
				renderQueue->colorWrite(false);
				renderQueue->clear(false, true);
				renderQueue->bind(shaderDepth);
				renderQueue->checkGlErrorDebug();

				renderEntities<RenderModeEnum::Shadowmap>(pass);

				renderQueue->resetFrameBuffer();
				renderQueue->resetAllState();
				renderQueue->resetAllTextures();
				renderQueue->checkGlErrorDebug();

				DebugVisualizationInfo deb;
				deb.texture = shadowmap.shadowTexture.share();
				deb.shader = shaderVisualizeDepth.share();
				debugVisualizations.push_back(std::move(deb));

				shadowmaps[le] = std::move(shadowmap);
			}

			void renderEffects(const TextureHandle texSource_, const TextureHandle depthTexture, const PassInfo &pass, const CameraComponent &cam, const uint32 cameraName)
			{
				const auto graphicsDebugScope = renderQueue->namedScope("effects");

				TextureHandle texSource = texSource_;
				TextureHandle texTarget = [&]() {
					TextureHandle t = provisionalData->texture(Stringizer() + "intermediateTarget_" + pass.resolution);
					renderQueue->bind(t, 0);
					renderQueue->filters(GL_LINEAR, GL_LINEAR, 0);
					renderQueue->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					renderQueue->image2d(pass.resolution, GL_RGB16F);
					return t;
				}();

				ScreenSpaceCommonConfig commonConfig; // helper to simplify initialization
				commonConfig.assets = engineAssets();
				commonConfig.provisionals = +provisionalData;
				commonConfig.queue = +renderQueue;
				commonConfig.resolution = pass.resolution;

				// depth of field
				if (any(cam.effects & CameraEffectsFlags::DepthOfField))
				{
					ScreenSpaceDepthOfFieldConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					(ScreenSpaceDepthOfField &)cfg = cam.depthOfField;
					cfg.proj = pass.proj;
					cfg.inDepth = depthTexture;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					screenSpaceDepthOfField(cfg);
					std::swap(texSource, texTarget);
				}

				// eye adaptation
				if (any(cam.effects & CameraEffectsFlags::EyeAdaptation))
				{
					ScreenSpaceEyeAdaptationConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					(ScreenSpaceEyeAdaptation &)cfg = cam.eyeAdaptation;
					cfg.cameraId = Stringizer() + cameraName;
					cfg.inColor = texSource;
					screenSpaceEyeAdaptationPrepare(cfg);
				}

				// bloom
				if (any(cam.effects & CameraEffectsFlags::Bloom))
				{
					ScreenSpaceBloomConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					(ScreenSpaceBloom &)cfg = cam.bloom;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					screenSpaceBloom(cfg);
					std::swap(texSource, texTarget);
				}

				// eye adaptation
				if (any(cam.effects & CameraEffectsFlags::EyeAdaptation))
				{
					ScreenSpaceEyeAdaptationConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					(ScreenSpaceEyeAdaptation &)cfg = cam.eyeAdaptation;
					cfg.cameraId = Stringizer() + cameraName;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					screenSpaceEyeAdaptationApply(cfg);
					std::swap(texSource, texTarget);
				}

				// final screen effects
				if (any(cam.effects & (CameraEffectsFlags::ToneMapping | CameraEffectsFlags::GammaCorrection)))
				{
					ScreenSpaceTonemapConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					(ScreenSpaceTonemap &)cfg = cam.tonemap;
					cfg.inColor = texSource;
					cfg.outColor = texTarget;
					cfg.tonemapEnabled = any(cam.effects & CameraEffectsFlags::ToneMapping);
					cfg.gamma = any(cam.effects & CameraEffectsFlags::GammaCorrection) ? cam.gamma : 1;
					screenSpaceTonemap(cfg);
					std::swap(texSource, texTarget);
				}

				// fxaa
				if (any(cam.effects & CameraEffectsFlags::AntiAliasing))
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
					renderQueue->viewport(Vec2i(), pass.resolution);
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

			void renderCamera(Entity *e, const CameraComponent &cam)
			{
				const auto graphicsDebugScope = renderQueue->namedScope("camera");

				PassInfo pass;
				pass.sceneMask = cam.sceneMask;
				pass.resolution = cam.target ? cam.target->resolution() : windowResolution;
				pass.model = modelTransform(e);
				pass.view = inverse(pass.model);
				pass.proj = [&]() {
					switch (cam.cameraType)
					{
					case CameraTypeEnum::Orthographic:
					{
						const Vec2 &os = cam.camera.orthographicSize;
						return orthographicProjection(-os[0], os[0], -os[1], os[1], cam.near, cam.far);
					}
					case CameraTypeEnum::Perspective: return perspectiveProjection(cam.camera.perspectiveFov, Real(pass.resolution[0]) / Real(pass.resolution[1]), cam.near, cam.far);
					default: CAGE_THROW_ERROR(Exception, "invalid camera type");
					}
				}();
				pass.viewProj = pass.proj * pass.view;
				initializeLodSelection(pass, cam, pass.model);

				const String texturesName = cam.target ? (Stringizer() + e->name()) : (Stringizer() + pass.resolution);
				TextureHandle colorTexture = [&]() {
					TextureHandle t = provisionalData->texture(Stringizer() + "colorTarget_" + texturesName);
					renderQueue->bind(t, 0);
					renderQueue->filters(GL_LINEAR, GL_LINEAR, 0);
					renderQueue->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					renderQueue->image2d(pass.resolution, GL_RGB16F);
					DebugVisualizationInfo deb;
					deb.texture = t;
					deb.shader = shaderVisualizeColor.share();
					debugVisualizations.push_back(std::move(deb));
					return t;
				}();
				TextureHandle depthTexture = [&]() {
					TextureHandle t = provisionalData->texture(Stringizer() + "depthTarget_" + texturesName);
					renderQueue->bind(t, 0);
					renderQueue->filters(GL_LINEAR, GL_LINEAR, 0);
					renderQueue->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					renderQueue->image2d(pass.resolution, GL_DEPTH_COMPONENT32);
					DebugVisualizationInfo deb;
					deb.texture = t;
					deb.shader = shaderVisualizeDepth.share();
					debugVisualizations.push_back(std::move(deb));
					return t;
				}();
				FrameBufferHandle renderTarget = provisionalData->frameBufferDraw("renderTarget");
				renderQueue->bind(renderTarget);
				renderQueue->colorTexture(0, colorTexture);
				renderQueue->depthTexture(depthTexture);
				renderQueue->activeAttachments(1);
				renderQueue->checkFrameBuffer();
				renderQueue->checkGlErrorDebug();

				renderQueue->viewport(Vec2i(), pass.resolution);
				{
					UniViewport viewport;
					viewport.vpInv = inverse(pass.viewProj);
					viewport.eyePos = pass.model * Vec4(0, 0, 0, 1);
					viewport.eyeDir = pass.model * Vec4(0, 0, -1, 0);
					viewport.ambientLight = Vec4(colorGammaToLinear(cam.ambientColor) * cam.ambientIntensity, 0);
					viewport.ambientDirectionalLight = Vec4(colorGammaToLinear(cam.ambientDirectionalColor) * cam.ambientDirectionalIntensity, 0);
					viewport.viewport = Vec4(Vec2(), Vec2(pass.resolution));
					renderQueue->universalUniformStruct(viewport, CAGE_SHADER_UNIBLOCK_VIEWPORT);
				}
				if (any(cam.clear))
					renderQueue->clear(any(cam.clear & CameraClearFlags::Color), any(cam.clear & CameraClearFlags::Depth), any(cam.clear & CameraClearFlags::Stencil));

				{
					const auto graphicsDebugScope = renderQueue->namedScope("depth prepass");
					renderEntities<RenderModeEnum::DepthPrepass>(pass);
				}
				{
					const auto graphicsDebugScope = renderQueue->namedScope("standard");
					renderEntities<RenderModeEnum::Standard>(pass);
				}

				renderQueue->resetAllState();
				renderQueue->resetAllTextures();
				renderEffects(colorTexture, depthTexture, pass, cam, e->name());

				{
					const auto graphicsDebugScope = renderQueue->namedScope("final blit");
					renderQueue->resetAllState();
					renderQueue->viewport(Vec2i(), pass.resolution);
					renderQueue->bind(modelSquare);
					renderQueue->bind(colorTexture, 0);
					renderQueue->bind(shaderBlit);
					if (cam.target)
					{ // blit to texture
						renderQueue->bind(renderTarget);
						renderQueue->colorTexture(0, Holder<Texture>(cam.target, nullptr));
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

			void run()
			{
				if (windowResolution[0] <= 0 || windowResolution[1] <= 0)
					return;
				if (!loadGeneralAssets())
					return;

				debugVisualizations.clear();
				std::vector<std::pair<Entity *, const CameraComponent *>> cameras;
				entitiesVisitor([&](Entity *e, const CameraComponent &cam) {
					cameras.emplace_back(e, &cam);
				}, +emit.scene, false);
				std::sort(cameras.begin(), cameras.end(), [](const auto &a, const auto &b) {
					return std::make_pair(!a.second->target, a.second->cameraOrder) < std::make_pair(!b.second->target, b.second->cameraOrder);
				});
				for (const auto &c : cameras)
				{
					shadowmaps.clear();
					entitiesVisitor([&](Entity *e, const LightComponent &lc, const ShadowmapComponent &sc) {
						renderShadowmap(c.first, *c.second, e, lc, sc);
					}, +emit.scene, false);
					renderCamera(c.first, *c.second);
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
