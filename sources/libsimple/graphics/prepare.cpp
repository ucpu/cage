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
			Mat4 mvpMat;
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

		struct Preparator
		{
			const Holder<RenderQueue> &renderQueue = graphics->renderQueue;
			const Holder<ProvisionalGraphics> &provisionalData = graphics->provisionalData;
			const Holder<SkeletalAnimationPreparatorCollection> skeletonPreparatorCollection = newSkeletalAnimationPreparatorCollection(engineAssets(), confRenderSkeletonBones);
			const Vec2i windowResolution = engineWindow()->resolution();
			const uint32 frameIndex = graphics->frameIndex;
			const bool cnfRenderMissingModels = confRenderMissingModels;
			const bool cnfRenderSkeletonBones = confRenderSkeletonBones;
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

				Holder<Texture> textures[MaxTexturesCountPerMaterial];
				uint32 shaderRoutines[CAGE_SHADER_MAX_ROUTINES] = {};

				for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
				{
					const uint32 n = model->textureName(i);
					if (!n)
						continue;
					textures[i] = engineAssets()->tryGet<AssetSchemeIndexTexture, Texture>(n);
				}
				updateShaderRoutinesForTextures(textures, shaderRoutines);

				//shaderRoutines[CAGE_SHADER_ROUTINEUNIF_SKELETON] = render.skeletalAnimation ? CAGE_SHADER_ROUTINEPROC_SKELETONANIMATION : CAGE_SHADER_ROUTINEPROC_SKELETONNOTHING;
				shaderRoutines[CAGE_SHADER_ROUTINEUNIF_SKELETON] = CAGE_SHADER_ROUTINEPROC_SKELETONNOTHING;

				UniInstance um = render.uni;
				if (none(model->flags & MeshRenderFlags::VelocityWrite))
					um.mvpMatPrev = um.mvpMat;
				um.normalMat.data[2][3] = any(model->flags & MeshRenderFlags::Lighting) ? 1 : 0; // is lighting enabled

				/*
				if (pr.textureAnimation)
				{
					Holder<Texture> t = engineAssets()->tryGet<AssetSchemeIndexTexture, Texture>(mo->textureName(CAGE_SHADER_TEXTURE_ALBEDO));
					if (t)
					{
						const auto &p = pr.textureAnimation->params;
						um.aniTexFrames = detail::evalSamplesForTextureAnimation(+t, prepareTime, p.startTime, p.speed, p.offset);
					}
				}

				if (pr.skeletalAnimation)
				{
					{
						ScopeLock lock(skeletonTaskInitMutex);
						if (!pr.skeletalAnimation->task)
							pr.skeletalAnimation->task = tasksRunAsync("skeletal-animation", Holder<PrepSkeleton>(+pr.skeletalAnimation, nullptr)); // the task may not own the skeleton otherwise it would make a cyclic dependency
					}
					armatures.push_back(+pr.skeletalAnimation);
					CAGE_ASSERT(armatures.size() == data.size());
				}
				*/

				renderQueue->bind(model);
				renderQueue->bind(shaderStandard);
				renderQueue->uniform(CAGE_SHADER_UNI_ROUTINES, shaderRoutines);
				for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
				{
					const Holder<Texture> &t = textures[i];
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
					renderQueue->culling(!any(model->flags & MeshRenderFlags::TwoSided));
					renderQueue->depthTest(any(model->flags & MeshRenderFlags::DepthTest));
					renderQueue->depthWrite(any(model->flags & MeshRenderFlags::DepthWrite));
				}
				renderQueue->universalUniformArray<UniInstance>({ &um, &um + 1 }, CAGE_SHADER_UNIBLOCK_MESHES);
				renderQueue->draw();
				renderQueue->checkGlErrorDebug();
			}

			template<bool DepthOnly>
			void renderObject(const PassInfo &pass, const RenderInfo &render, Holder<RenderObject> object)
			{
				CAGE_ASSERT(object->lodsCount() > 0);
				uint32 lod = 0;
				/*
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
				*/
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
			}

			template<bool DepthOnly>
			void renderEntities(const PassInfo &pass)
			{
				entitiesVisitor([&](Entity *e, const RenderComponent &rc) {
					if ((rc.sceneMask & pass.sceneMask) == 0)
						return;
					RenderInfo render;
					render.e = e;
					const auto model = modelTransforms(e);
					render.model = Mat4(model.first);
					render.modelPrev = Mat4(model.second);
					render.render = rc;
					render.uni.color = Vec4(colorGammaToLinear(render.render.color) * render.render.intensity, render.render.opacity);
					render.uni.mMat = Mat3x4(render.model);
					render.uni.mvpMat = pass.viewProj * render.model;
					render.uni.mvpMatPrev = pass.viewProjPrev * render.modelPrev;
					render.uni.normalMat = Mat3x4(inverse(Mat3(render.model)));
					render.frustum = Frustum(render.uni.mvpMat);
					renderObjectOrModel<DepthOnly>(pass, render, rc.object);
				}, +emit.scene, false);
			}

			// current and previous transformation matrices
			std::pair<Transform, Transform> modelTransforms(Entity *e)
			{
				CAGE_ASSERT(e->has(transformComponent));
				if (e->has(prevTransformComponent))
				{
					const Transform c = e->value<TransformComponent>(transformComponent);
					const Transform p = e->value<TransformComponent>(prevTransformComponent);
					return { interpolate(p, c, interpolationFactor), interpolate(p, c, interpolationFactor - 1) };
				}
				else
				{
					const Transform t = e->value<TransformComponent>(transformComponent);
					return { t, t };
				}
			}

			void run()
			{
				if (windowResolution[0] <= 0 || windowResolution[1] <= 0)
					return;
				if (!loadGeneralAssets())
					return;

				// render shadow maps
				entitiesVisitor([&](Entity *e, const LightComponent &lc, const ShadowmapComponent &sc) {
					// todo
				}, +emit.scene, false);

				// render cameras
				entitiesVisitor([&](Entity *e, const CameraComponent &cam) {
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
