#include <cage-core/skeletalAnimationPreparator.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-core/blockContainer.h>
#include <cage-core/concurrent.h>
#include <cage-core/hashString.h>
#include <cage-core/profiling.h>
#include <cage-core/geometry.h>
#include <cage-core/textPack.h>
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

#include <robin_hood.h>
#include <algorithm>
#include <optional>

namespace cage
{
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

		struct UniInstance
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
			Vec4 color; // + spotAngle
			Vec4 position;
			Vec4 direction; // + normalOffsetScale
			Vec4 attenuation; // + spotExponent
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
			UniInstance uni;
			Mat4 model, modelPrev;
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
		};

		// camera or light
		struct PassInfo
		{
			Mat4 model, modelPrev;
			Mat4 view, viewPrev;
			Mat4 viewProj, viewProjPrev;
			Mat4 proj;
			Vec2i resolution;
			Entity *e = nullptr;
			uint32 sceneMask = 0;

			struct LodSelection
			{
				Vec3 center; // center of camera (NOT light)
				Real screenSize = 0; // vertical size of screen in pixels, one meter in front of the camera
				bool orthographic = false;
			} lodSelection;
		};

		void RenderInfo::initUniMatrices(const struct PassInfo &pass)
		{
			uni.mMat = Mat3x4(model);
			uni.mvpMat = pass.viewProj * model;
			uni.mvpMatPrev = pass.viewProjPrev * modelPrev;
			uni.normalMat = Mat3x4(inverse(Mat3(model)));
		}

		struct Preparator
		{
			const bool cnfRenderMissingModels = confRenderMissingModels;
			const bool cnfRenderSkeletonBones = confRenderSkeletonBones;
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

			template<bool DepthOnly>
			void renderModelImpl(const PassInfo &pass, RenderInfoEx &ex, Holder<Model> model)
			{
				renderQueue->bind(model);
				renderQueue->bind(DepthOnly ? shaderDepth : shaderStandard);
				renderQueue->culling(!any(model->flags & MeshRenderFlags::TwoSided));
				renderQueue->depthTest(any(model->flags & MeshRenderFlags::DepthTest));
				renderQueue->depthWrite(any(model->flags & MeshRenderFlags::DepthWrite));

				if (none(model->flags & MeshRenderFlags::VelocityWrite))
					ex.uni.mvpMatPrev = ex.uni.mvpMat;
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

				ex.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_SKELETON] = ex.skeletalAnimation ? CAGE_SHADER_ROUTINEPROC_SKELETONANIMATION : CAGE_SHADER_ROUTINEPROC_SKELETONNOTHING;
				if (ex.skeletalAnimation)
				{
					const auto armature = ex.skeletalAnimation->armature();
					CAGE_ASSERT(armature.size() == model->skeletonBones);
					renderQueue->uniform(CAGE_SHADER_UNI_BONESPERINSTANCE, model->skeletonBones);
					renderQueue->universalUniformArray(armature, CAGE_SHADER_UNIBLOCK_ARMATURES);
				}

				renderQueue->universalUniformArray<UniInstance>({ &ex.uni, &ex.uni + 1 }, CAGE_SHADER_UNIBLOCK_MESHES);
				if constexpr (DepthOnly)
					renderQueue->blending(false);
				else
				{
					ex.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE] = CAGE_SHADER_ROUTINEPROC_LIGHTFORWARDBASE;
					renderQueue->blending(any(model->flags & MeshRenderFlags::Translucent));
					renderQueue->blendFuncPremultipliedTransparency();
				}
				renderQueue->uniform(CAGE_SHADER_UNI_ROUTINES, ex.shaderRoutines);
				renderQueue->draw();

				if constexpr (!DepthOnly)
				{
					renderQueue->depthFuncLessEqual();
					renderQueue->blending(true);
					renderQueue->blendFuncAdditive();
					entitiesVisitor([&](Entity *e, const LightComponent &lc) {
						if ((lc.sceneMask & pass.sceneMask) == 0)
							return;
						const auto model = modelTransforms(e);
						UniLight uni;
						uni.color = Vec4(colorGammaToLinear(lc.color) * lc.intensity, cos(lc.spotAngle * 0.5));
						uni.position = model.first * Vec4(0, 0, 0, 1);
						uni.direction = model.first * Vec4(0, 0, -1, 0);
						uni.attenuation = Vec4(lc.attenuation, lc.spotExponent);
						switch (lc.lightType)
						{
						case LightTypeEnum::Directional:
							//shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE] = shadows ? CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONALSHADOW : CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONAL;
							ex.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE] = CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONAL;
							break;
						case LightTypeEnum::Spot:
							//shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE] = shadows ? CAGE_SHADER_ROUTINEPROC_LIGHTSPOTSHADOW : CAGE_SHADER_ROUTINEPROC_LIGHTSPOT;
							ex.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE] = CAGE_SHADER_ROUTINEPROC_LIGHTSPOT;
							break;
						case LightTypeEnum::Point:
							//shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE] = shadows ? CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW : CAGE_SHADER_ROUTINEPROC_LIGHTPOINT;
							ex.shaderRoutines[CAGE_SHADER_ROUTINEUNIF_LIGHTTYPE] = CAGE_SHADER_ROUTINEPROC_LIGHTPOINT;
							break;
						default:
							CAGE_THROW_CRITICAL(Exception, "invalid light type");
						}
						renderQueue->universalUniformStruct<UniLight>(uni, CAGE_SHADER_UNIBLOCK_LIGHTS);
						renderQueue->uniform(CAGE_SHADER_UNI_ROUTINES, ex.shaderRoutines);
						renderQueue->draw();
					}, +emit.scene, false);
					renderQueue->depthFuncLess();
				}

				renderQueue->checkGlErrorDebug();
			}

			template<bool DepthOnly>
			void renderModelBones(const PassInfo &pass, const RenderInfoEx &ex)
			{
				const auto armature = ex.skeletalAnimation->armature();
				for (uint32 i = 0; i < armature.size(); i++)
				{
					const Mat4 am = Mat4(armature[i]);
					RenderInfoEx r;
					(RenderInfo &)r = (const RenderInfo &)ex;
					r.model = ex.model * am;
					r.modelPrev = ex.modelPrev * am;
					r.initUniMatrices(pass);
					r.uni.color = Vec4(colorGammaToLinear(colorHsvToRgb(Vec3(Real(i) / Real(armature.size()), 1, 1))), r.render.opacity);
					renderModelImpl<DepthOnly>(pass, r, modelBone.share());
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
					shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPALBEDO] = CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING;

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
					shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPSPECIAL] = CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING;

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
					shaderRoutines[CAGE_SHADER_ROUTINEUNIF_MAPNORMAL] = CAGE_SHADER_ROUTINEPROC_MATERIALNOTHING;
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

			template<bool DepthOnly>
			void renderModel(const PassInfo &pass, const RenderInfo &render, Holder<Model> model, Holder<RenderObject> parent = {})
			{
				if constexpr (DepthOnly)
				{
					if (none(model->flags & MeshRenderFlags::ShadowCast))
						return;
				}

				if (!intersects(model->boundingBox(), render.frustum))
					return;

				RenderInfoEx ex;
				(RenderInfo &)ex = render;
				updateRenderInfo(ex, +parent);

				if (cnfRenderSkeletonBones && ex.skeletalAnimation)
					renderModelBones<DepthOnly>(pass, ex);
				else
					renderModelImpl<DepthOnly>(pass, ex, std::move(model));
			}

			template<bool DepthOnly>
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
					renderObjectOrModel<DepthOnly>(pass, render, it, object.share());
			}

			template<bool DepthOnly>
			void renderObjectOrModel(const PassInfo &pass, const RenderInfo &render, const uint32 name, Holder<RenderObject> parent = {})
			{
				Holder<RenderObject> obj = engineAssets()->tryGet<AssetSchemeIndexRenderObject, RenderObject>(name);
				if (obj)
				{
					renderObject<DepthOnly>(pass, render, std::move(obj));
					return;
				}
				Holder<Model> md = engineAssets()->tryGet<AssetSchemeIndexModel, Model>(name);
				if (md)
				{
					renderModel<DepthOnly>(pass, render, std::move(md), std::move(parent));
					return;
				}
				if (cnfRenderMissingModels)
				{
					Holder<Model> fake = engineAssets()->tryGet<AssetSchemeIndexModel, Model>(HashString("cage/model/fake.obj"));
					renderModel<DepthOnly>(pass, render, std::move(fake), std::move(parent));
					return;
				}
			}

			template<bool DepthOnly>
			void renderEntities(const PassInfo &pass)
			{
				const auto graphicsDebugScope = renderQueue->namedScope("render entities");
				entitiesVisitor([&](Entity *e, const RenderComponent &rc) {
					if ((rc.sceneMask & pass.sceneMask) == 0)
						return;
					RenderInfo render;
					render.e = e;
					const auto model = modelTransforms(e);
					render.model = model.first;
					render.modelPrev = model.second;
					render.render = rc;
					render.initUniMatrices(pass);
					render.frustum = Frustum(render.uni.mvpMat);
					renderObjectOrModel<DepthOnly>(pass, render, rc.object);
				}, +emit.scene, false);
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

			// current and previous transformation matrices
			std::pair<Mat4, Mat4> modelTransforms(Entity *e)
			{
				CAGE_ASSERT(e->has(transformComponent));
				if (e->has(prevTransformComponent))
				{
					const Transform c = e->value<TransformComponent>(transformComponent);
					const Transform p = e->value<TransformComponent>(prevTransformComponent);
					return { Mat4(interpolate(p, c, interpolationFactor)), Mat4(interpolate(p, c, interpolationFactor - 1)) };
				}
				else
				{
					const Mat4 t = Mat4(e->value<TransformComponent>(transformComponent));
					return { t, t };
				}
			}

			TextureHandle prepareTargetTexture(const char *name, uint32 internalFormat, const Vec2i &resolution)
			{
				TextureHandle t = provisionalData->texture(Stringizer() + name + resolution[0]);
				renderQueue->bind(t, 0);
				renderQueue->filters(GL_LINEAR, GL_LINEAR, 0);
				renderQueue->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
				renderQueue->image2d(resolution, internalFormat);
				return t;
			}

			void run()
			{
				if (windowResolution[0] <= 0 || windowResolution[1] <= 0)
					return;
				if (!loadGeneralAssets())
					return;

				// render shadow maps
				entitiesVisitor([&](Entity *e, const LightComponent &lc, const ShadowmapComponent &sc) {
					const auto graphicsDebugScope = renderQueue->namedScope("shadowmap");
					// todo
				}, +emit.scene, false);

				// render cameras
				entitiesVisitor([&](Entity *e, const CameraComponent &cam) {
					const auto graphicsDebugScope = renderQueue->namedScope("camera");
					PassInfo pass;
					pass.e = e;
					pass.sceneMask = cam.sceneMask;
					pass.resolution = cam.target ? cam.target->resolution() : windowResolution;
					const auto model = modelTransforms(e);
					pass.model = Mat4(model.first);
					pass.modelPrev = Mat4(model.second);
					pass.view = inverse(pass.model);
					pass.viewPrev = inverse(pass.modelPrev);
					pass.proj = [&]() {
						switch (cam.cameraType)
						{
						case CameraTypeEnum::Orthographic:
						{
							const Vec2 &os = cam.camera.orthographicSize;
							return orthographicProjection(-os[0], os[0], -os[1], os[1], cam.near, cam.far);
						}
						case CameraTypeEnum::Perspective:
							return perspectiveProjection(cam.camera.perspectiveFov, Real(pass.resolution[0]) / Real(pass.resolution[1]), cam.near, cam.far);
						default:
							CAGE_THROW_ERROR(Exception, "invalid camera type");
						}
					}();
					pass.viewProj = pass.proj * pass.view;
					pass.viewProjPrev = pass.proj * pass.viewPrev;
					initializeLodSelection(pass, cam, pass.model);

					TextureHandle colorTexture = prepareTargetTexture("colorTarget", GL_RGB16F, pass.resolution);
					TextureHandle depthTexture = prepareTargetTexture("depthTarget", GL_DEPTH_COMPONENT32, pass.resolution);
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

					renderEntities<false>(pass);

					{
						const auto graphicsDebugScope = renderQueue->namedScope("final blit");
						renderQueue->resetAllState();
						if (cam.target)
						{ // blit to texture
							renderQueue->bind(shaderBlit);
							renderQueue->bind(renderTarget);
							renderQueue->colorTexture(0, Holder<Texture>(cam.target, nullptr));
							renderQueue->depthTexture({});
							renderQueue->activeAttachments(1);
							renderQueue->checkFrameBuffer();
						}
						else
						{ // blit to window
							renderQueue->bind(shaderBlit);
							renderQueue->resetFrameBuffer();
						}
						renderQueue->viewport(Vec2i(), pass.resolution);
						renderQueue->bind(modelSquare);
						renderQueue->bind(colorTexture, 0);
						renderQueue->draw();
						renderQueue->resetFrameBuffer();
						renderQueue->resetAllTextures();
						renderQueue->resetAllState();
						renderQueue->checkGlErrorDebug();
					}
				}, +emit.scene, false);
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
