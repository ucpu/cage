#include <cage-core/skeletalAnimationPreparator.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-core/hashString.h>
#include <cage-core/profiling.h>
#include <cage-core/textPack.h>
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
#include <array>

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

		struct ModelShared
		{
			Holder<Model> mesh;
			bool skeletal = false;

			bool operator < (const ModelShared &other) const
			{
				return std::tuple{ +mesh, skeletal } < std::tuple{ +other.mesh, other.skeletal };
			}
		};

		struct ModelInstance
		{
			UniMesh uni;
			Holder<SkeletalAnimationPreparatorInstance> skeletalAnimation;
		};

		struct ModelTranslucent : public ModelShared, public ModelInstance
		{
			Real depth = Real::Nan(); // used to sort translucent back-to-front
		};

		struct ModelPrepare : public ModelTranslucent
		{
			Mat4 model;
			Frustum frustum; // object-space camera frustum used for culling
			RenderComponent render;
			std::optional<TextureAnimationComponent> textureAnimation;
			Entity *e = nullptr;
			bool translucent = false;

			ModelPrepare clone() const
			{
				ModelPrepare res;
				res.mesh = mesh.share();
				res.translucent = translucent;
				res.uni = uni;
				res.skeletalAnimation = skeletalAnimation.share();
				res.model = model;
				res.frustum = frustum;
				res.render = render;
				res.textureAnimation = textureAnimation;
				res.e = e;
				return res;
			}
		};

		struct TextPrepare
		{
			Mat4 model;
			Holder<const Font> font;
			std::vector<uint32> glyphs;
			FontFormat format;
			Vec3 color;
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

			std::map<ModelShared, std::vector<ModelInstance>> opaque;
			std::vector<ModelTranslucent> translucent;
			std::vector<TextPrepare> texts;
			std::vector<DebugVisualizationInfo> debugVisualizations;
			Holder<RenderQueue> renderQueue;

			std::map<Entity *, struct ShadowmapData> shadowmaps;
			UubRange lighsBlock;
			uint32 lightsCount = 0;
		};

		struct ShadowmapData : public CommonData
		{
			Mat4 shadowMat;
			Holder<ProvisionalTexture> shadowTexture;
			LightComponent lightComponent;
			ShadowmapComponent shadowmapComponent;
		};

		struct CameraData : public CommonData
		{
			CameraComponent camera;
		};

		UniMesh initializeMeshUni(const CommonData &data, const Mat4 model)
		{
			UniMesh uni;
			uni.mMat = Mat3x4(model);
			uni.mvpMat = data.viewProj * model;
			uni.normalMat = Mat3x4(inverse(Mat3(model)));
			return uni;
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

		void updateShaderRoutinesForTextures(const std::array<Holder<Texture>, MaxTexturesCountPerMaterial> &textures, std::array<uint32, CAGE_SHADER_MAX_ROUTINES> &shaderRoutines)
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

		template<class T>
		PointerRange<T> subRange(PointerRange<T> base, uint32 off, uint32 num)
		{
			CAGE_ASSERT(off + num <= base.size());
			return { base.begin() + off, base.begin() + off + num };
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
			void renderModelsImpl(const CommonData &data, const ModelShared &sh, const PointerRange<const UniMesh> uniMeshes, const PointerRange<const Mat3x4> uniArmatures, bool translucent) const
			{
				const Holder<RenderQueue> &renderQueue = data.renderQueue;
				renderQueue->bind(sh.mesh);
				renderQueue->bind(RenderMode == RenderModeEnum::Standard ? shaderStandard : shaderDepth);
				renderQueue->culling(!any(sh.mesh->flags & MeshRenderFlags::TwoSided));
				renderQueue->depthTest(any(sh.mesh->flags & MeshRenderFlags::DepthTest));
				renderQueue->depthWrite(any(sh.mesh->flags & MeshRenderFlags::DepthWrite));
				std::array<Holder<Texture>, MaxTexturesCountPerMaterial> textures = {};
				std::array<uint32, CAGE_SHADER_MAX_ROUTINES> shaderRoutines = {};
				for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
				{
					const uint32 n = sh.mesh->textureName(i);
					if (!n)
						continue;
					textures[i] = engineAssets()->tryGet<AssetSchemeIndexTexture, Texture>(n);
					switch (textures[i]->target())
					{
					case GL_TEXTURE_2D_ARRAY: renderQueue->bind(textures[i], CAGE_SHADER_TEXTURE_ALBEDO_ARRAY + i); break;
					case GL_TEXTURE_CUBE_MAP: renderQueue->bind(textures[i], CAGE_SHADER_TEXTURE_ALBEDO_CUBE + i); break;
					default: renderQueue->bind(textures[i], CAGE_SHADER_TEXTURE_ALBEDO + i); break;
					}
				}
				updateShaderRoutinesForTextures(textures, shaderRoutines);
				renderQueue->uniform(CAGE_SHADER_UNI_BONESPERINSTANCE, sh.mesh->skeletonBones);
				shaderRoutines[CAGE_SHADER_ROUTINEUNIF_SKELETON] = sh.skeletal ? 1 : 0;
				shaderRoutines[CAGE_SHADER_ROUTINEUNIF_OPAQUE] = translucent ? 0 : 1;
				renderQueue->uniform(CAGE_SHADER_UNI_ROUTINES, shaderRoutines);

				const uint32 limit = sh.skeletal ? min(uint32(CAGE_SHADER_MAX_MESHES), CAGE_SHADER_MAX_BONES / sh.mesh->skeletonBones) : CAGE_SHADER_MAX_MESHES;
				for (uint32 offset = 0; offset < uniMeshes.size(); offset += limit)
				{
					const uint32 count = min(limit, numeric_cast<uint32>(uniMeshes.size()) - offset);
					renderQueue->universalUniformArray<const UniMesh>(subRange<const UniMesh>(uniMeshes, offset, count), CAGE_SHADER_UNIBLOCK_MESHES);
					if (sh.skeletal)
						renderQueue->universalUniformArray<const Mat3x4>(subRange<const Mat3x4>(uniArmatures, offset * sh.mesh->skeletonBones, count * sh.mesh->skeletonBones), CAGE_SHADER_UNIBLOCK_ARMATURES);

					if constexpr (RenderMode == RenderModeEnum::Standard)
					{
						renderQueue->depthFuncLessEqual();
						renderQueue->blending(translucent);
						renderQueue->blendFuncPremultipliedTransparency();
						if (data.lightsCount > 0)
							renderQueue->bind(data.lighsBlock, CAGE_SHADER_UNIBLOCK_LIGHTS);
						renderQueue->uniform(CAGE_SHADER_UNI_LIGHTSCOUNT, data.lightsCount);
						renderQueue->draw(count);

						renderQueue->depthFuncLessEqual();
						renderQueue->blending(true);
						renderQueue->blendFuncAdditive();
						renderQueue->uniform(CAGE_SHADER_UNI_LIGHTSCOUNT, uint32(1));
						for (const auto &smit : data.shadowmaps)
						{
							const ShadowmapData &sm = smit.second;
							renderQueue->bind(sm.shadowTexture, sm.lightComponent.lightType == LightTypeEnum::Point ? CAGE_SHADER_TEXTURE_SHADOW_CUBE : CAGE_SHADER_TEXTURE_SHADOW);
							renderQueue->bind(sm.lighsBlock, CAGE_SHADER_UNIBLOCK_LIGHTS);
							renderQueue->uniform(CAGE_SHADER_UNI_SHADOWMATRIX, sm.shadowMat);
							renderQueue->draw(count);
						}
					}
					else
					{
						renderQueue->depthFuncLess();
						renderQueue->blending(false);
						renderQueue->draw(count);
					}
				}

				renderQueue->checkGlErrorDebug();
			}

			void renderTextImpl(const CommonData &data, const TextPrepare &text) const
			{
				const Holder<RenderQueue> &renderQueue = data.renderQueue;
				text.font->bind(+renderQueue, modelSquare, shaderFont);
				renderQueue->uniform(0, data.viewProj * text.model);
				renderQueue->uniform(4, text.color);
				text.font->render(+renderQueue, text.glyphs, text.format);
			}

			template<RenderModeEnum RenderMode>
			void renderModels(const CommonData &data) const
			{
				const Holder<RenderQueue> &renderQueue = data.renderQueue;

				{
					const auto graphicsDebugScope = renderQueue->namedScope("opaque");
					for (const auto &shit : data.opaque)
					{
						const ModelShared &sh = shit.first;
						std::vector<UniMesh> uniMeshes;
						uniMeshes.reserve(shit.second.size());
						for (const ModelInstance &inst : shit.second)
							uniMeshes.push_back(inst.uni);
						std::vector<Mat3x4> uniArmatures;
						if (sh.skeletal)
						{
							uniArmatures.reserve(shit.second.size() * sh.mesh->skeletonBones);
							for (const ModelInstance &inst : shit.second)
							{
								const auto armature = inst.skeletalAnimation->armature();
								CAGE_ASSERT(armature.size() == sh.mesh->skeletonBones);
								for (const auto &it : armature)
									uniArmatures.push_back(it);
							}
						}
						renderModelsImpl<RenderMode>(data, sh, uniMeshes, uniArmatures, false);
					}
					renderQueue->resetAllState();
					renderQueue->viewport(Vec2i(), data.resolution);
				}

				if constexpr (RenderMode != RenderModeEnum::DepthPrepass)
				{
					const auto graphicsDebugScope = renderQueue->namedScope("translucent");
					for (const auto &it : data.translucent)
					{
						PointerRange<const UniMesh> uniMeshes = { &it.uni, &it.uni + 1 };
						PointerRange<const Mat3x4> uniArmatures;
						if (it.skeletal)
						{
							uniArmatures = it.skeletalAnimation->armature();
							CAGE_ASSERT(uniArmatures.size() == it.mesh->skeletonBones);
						}
						renderModelsImpl<RenderMode>(data, it, uniMeshes, uniArmatures, true);
					}
					renderQueue->resetAllState();
					renderQueue->viewport(Vec2i(), data.resolution);
				}

				if constexpr (RenderMode == RenderModeEnum::Standard)
				{
					const auto graphicsDebugScope = renderQueue->namedScope("texts");
					renderQueue->depthTest(true);
					renderQueue->depthWrite(false);
					renderQueue->culling(true);
					renderQueue->blending(true);
					renderQueue->blendFuncAlphaTransparency();
					for (const TextPrepare &text : data.texts)
						renderTextImpl(data, text);
					renderQueue->resetAllState();
					renderQueue->viewport(Vec2i(), data.resolution);
				}
			}

			void prepareModelImpl(CommonData &data, ModelPrepare &prepare) const
			{
				if (prepare.translucent)
				{
					ModelTranslucent &tr = (ModelTranslucent &)prepare;
					data.translucent.push_back(std::move(tr));
				}
				else
				{
					ModelShared &sh = (ModelShared &)prepare;
					ModelInstance &inst = (ModelInstance &)prepare;
					data.opaque[std::move(sh)].push_back(std::move(inst));
				}
			}

			void prepareModelBones(CommonData &data, const ModelPrepare &prepare) const
			{
				const auto armature = prepare.skeletalAnimation->armature();
				for (uint32 i = 0; i < armature.size(); i++)
				{
					ModelPrepare pr = prepare.clone();
					pr.skeletalAnimation.clear();
					pr.skeletal = false;
					pr.mesh = modelBone.share();
					pr.model = prepare.model * Mat4(armature[i]);
					pr.uni = initializeMeshUni(data, pr.model);
					pr.uni.color = Vec4(colorGammaToLinear(colorHsvToRgb(Vec3(Real(i) / Real(armature.size()), 1, 1))), 1);
					prepareModelImpl(data, pr);
				}
			}

			void prepareModel(CommonData &data, ModelPrepare &pr, Holder<RenderObject> parent = {}) const
			{
				if (!intersects(pr.mesh->boundingBox(), pr.frustum))
					return;

				std::optional<TextureAnimationComponent> &pt = pr.textureAnimation;
				if (pr.e->has<TextureAnimationComponent>())
					pt = pr.e->value<TextureAnimationComponent>();

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
						pt = TextureAnimationComponent();
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
					if (Holder<Texture> tex = engineAssets()->tryGet<AssetSchemeIndexTexture, Texture>(pr.mesh->textureName(0)))
						pr.uni.aniTexFrames = detail::evalSamplesForTextureAnimation(+tex, prepareTime, pt->startTime, pt->speed, pt->offset);
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
						pr.skeletalAnimation->prepare();
						pr.skeletal = true;
					}
				}
				CAGE_ASSERT(!!pr.skeletal == !!pr.skeletalAnimation);

				pr.translucent = any(pr.mesh->flags & MeshRenderFlags::Translucent) || pr.render.opacity < 1;
				pr.uni.normalMat.data[2][3] = any(pr.mesh->flags & MeshRenderFlags::Lighting) ? 1 : 0; // is lighting enabled

				pr.depth = (pr.uni.mvpMat * Vec4(0, 0, 0, 1))[2];

				if (pr.skeletal && cnfRenderSkeletonBones)
					prepareModelBones(data, pr);
				else
					prepareModelImpl(data, pr);
			}

			void prepareObject(CommonData &data, const ModelPrepare &prepare, Holder<RenderObject> object) const
			{
				CAGE_ASSERT(object->lodsCount() > 0);
				uint32 lod = 0;
				if (object->lodsCount() > 1)
				{
					Real d = 1;
					if (!data.lodSelection.orthographic)
					{
						const Vec4 ep4 = prepare.model * Vec4(0, 0, 0, 1);
						CAGE_ASSERT(abs(ep4[3] - 1) < 1e-4);
						d = distance(Vec3(ep4), data.lodSelection.center);
					}
					const Real f = data.lodSelection.screenSize * object->worldSize / (d * object->pixelsSize);
					lod = object->lodSelect(f);
				}
				for (uint32 it : object->models(lod))
				{
					ModelPrepare pr = prepare.clone();
					pr.mesh = engineAssets()->tryGet<AssetSchemeIndexModel, Model>(it);
					prepareModel(data, pr, object.share());
				}
			}

			void prepareEntities(CommonData &data) const
			{
				entitiesVisitor([&](Entity *e, const RenderComponent &rc) {
					if ((rc.sceneMask & data.sceneMask) == 0)
						return;
					ModelPrepare prepare;
					prepare.e = e;
					prepare.render = rc;
					prepare.model = modelTransform(e);
					prepare.uni = initializeMeshUni(data, prepare.model);
					prepare.frustum = Frustum(prepare.uni.mvpMat);
					if (Holder<RenderObject> obj = engineAssets()->tryGet<AssetSchemeIndexRenderObject, RenderObject>(rc.object))
					{
						prepareObject(data, prepare, std::move(obj));
						return;
					}
					if (Holder<Model> mesh = engineAssets()->tryGet<AssetSchemeIndexModel, Model>(rc.object))
					{
						prepare.mesh = std::move(mesh);
						prepareModel(data, prepare);
						return;
					}
					if (cnfRenderMissingModels)
					{
						prepare.mesh = engineAssets()->tryGet<AssetSchemeIndexModel, Model>(HashString("cage/model/fake.obj"));
						prepareModel(data, prepare);
						return;
					}
				}, +emit.scene, false);

				// sort translucent back-to-front
				std::sort(data.translucent.begin(), data.translucent.end(), [](const auto &a, const auto &b) { return a.depth > b.depth; });

				entitiesVisitor([&](Entity *e, const TextComponent &tc_) {
					if ((tc_.sceneMask & data.sceneMask) == 0)
						return;
					TextComponent pt = tc_;
					TextPrepare prepare;
					if (!pt.font)
						pt.font = HashString("cage/font/ubuntu/Ubuntu-R.ttf");
					prepare.font = engineAssets()->tryGet<AssetSchemeIndexFont, Font>(pt.font);
					if (!prepare.font)
						return;
					const String str = loadFormattedString(engineAssets(), pt.assetName, pt.textName, pt.value);
					const uint32 count = prepare.font->glyphsCount(str);
					if (count == 0)
						return;
					prepare.glyphs.resize(count);
					prepare.font->transcript(str, prepare.glyphs);
					prepare.color = colorGammaToLinear(pt.color) * pt.intensity;
					prepare.format.size = 1;
					const Vec2 size = prepare.font->size(prepare.glyphs, prepare.format);
					prepare.format.wrapWidth = size[0];
					prepare.model = modelTransform(e) * Mat4(Vec3(size * Vec2(-0.5, 0.5), 0));
					data.texts.push_back(std::move(prepare));
				}, +emit.scene, false);
			}

			void renderShadowmap(ShadowmapData &data, uint32) const
			{
				data.renderQueue = newRenderQueue(Stringizer() + data.entity);
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

				prepareEntities(data);
				renderModels<RenderModeEnum::Shadowmap>(data);

				renderQueue->resetFrameBuffer();
				renderQueue->resetAllState();
				renderQueue->resetAllTextures();
				renderQueue->checkGlErrorDebug();

				DebugVisualizationInfo deb;
				deb.texture = data.shadowTexture.share();
				deb.shader = shaderVisualizeDepth.share();
				data.debugVisualizations.push_back(std::move(deb));
			}

			void prepareCameraLights(CameraData &data) const
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

			void renderCameraEffects(const TextureHandle texSource_, const TextureHandle depthTexture, CameraData &data) const
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

			void renderCamera(CameraData &data, uint32) const
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
				prepareEntities(data);
				prepareCameraLights(data);

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
					renderModels<RenderModeEnum::DepthPrepass>(data);
				}
				{
					const auto graphicsDebugScope = renderQueue->namedScope("standard");
					renderModels<RenderModeEnum::Standard>(data);
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

			Holder<AsyncTask> prepareShadowmap(CameraData &camera, ShadowmapData &data) const
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

			void prepareCamera(CameraData &data) const
			{
				data.renderQueue = newRenderQueue(Stringizer() + data.entity);
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
