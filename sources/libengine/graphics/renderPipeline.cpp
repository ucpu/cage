#include <cage-core/skeletalAnimationPreparator.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/assetManager.h>
#include <cage-core/hashString.h>
#include <cage-core/meshImport.h>
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
#include <cage-engine/renderPipeline.h>
#include <cage-engine/shaderProgram.h>
#include <cage-engine/renderObject.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/texture.h>
#include <cage-engine/window.h>
#include <cage-engine/opengl.h> // all the constants
#include <cage-engine/model.h>
#include <cage-engine/scene.h>
#include <cage-engine/font.h>

#include <algorithm>
#include <optional>
#include <vector>
#include <map>
#include <array>

namespace cage
{
	namespace
	{
		const ConfigBool confRenderMissingModels("cage/graphics/renderMissingModels", false);
		const ConfigBool confRenderSkeletonBones("cage/graphics/renderSkeletonBones", false);
		const ConfigBool confNoAmbientOcclusion("cage/graphics/disableAmbientOcclusion", false);
		const ConfigBool confNoBloom("cage/graphics/disableBloom", false);

		struct UniMesh
		{
			Mat4 mvpMat;
			Mat3x4 normalMat; // [2][3] is 1 if lighting is enabled; [1][3] is 1 if transparent mode is fading
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
			Mat4 vMat; // view matrix
			Mat4 pMat; // proj matrix
			Mat4 vpMat; // viewProj matrix
			Mat4 vpInv; // viewProj inverse matrix
			Vec4 eyePos;
			Vec4 eyeDir;
			Vec4 viewport; // x, y, w, h
			Vec4 ambientLight; // color rgb is linear, no alpha
			Vec4 ambientDirectionalLight; // color rgb is linear, no alpha
			Vec4 time; // frame index (loops at 10000), time (loops every second), time (loops every 1000 seconds)
		};

		struct ModelShared
		{
			Holder<Model> mesh;
			bool skeletal = false;

			auto cmp() const
			{
				// reduce switching shaders, then depth test/writes and other states, then meshes
				return std::tuple{ mesh->shaderName, mesh->flags, +mesh, skeletal };
			}

			bool operator < (const ModelShared &other) const
			{
				return cmp() < other.cmp();
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
			bool translucent = false; // transparent or fade

			ModelPrepare clone() const
			{
				ModelPrepare res;
				res.mesh = mesh.share();
				res.skeletal = skeletal;
				res.uni = uni;
				res.skeletalAnimation = skeletalAnimation.share();
				res.depth = depth;
				res.model = model;
				res.frustum = frustum;
				res.render = render;
				res.textureAnimation = textureAnimation;
				res.e = e;
				res.translucent = translucent;
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

		struct DataLayer
		{
			std::map<ModelShared, std::vector<ModelInstance>> opaque;
			std::vector<ModelTranslucent> translucent;
			std::vector<TextPrepare> texts;
		};

		// camera or light
		struct CameraData : public RenderPipelineCamera
		{
			Mat4 model;
			Mat4 view;
			Mat4 viewProj;

			std::map<sint32, DataLayer> layers;
			PointerRangeHolder<RenderPipelineDebugVisualization> debugVisualizations;
			Holder<RenderQueue> renderQueue;

			std::map<Entity *, struct ShadowmapData> shadowmaps;
			uint32 lightsCount = 0;
		};

		struct ShadowmapData : public CameraData
		{
			UniLight shadowUni;
			Mat4 shadowMat;
			Holder<ProvisionalTexture> shadowTexture;
			LightComponent lightComponent;
			ShadowmapComponent shadowmapComponent;
		};

		UniMesh initializeMeshUni(const CameraData &data, const Mat4 model)
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
			uni.attenuation = lc.lightType == LightTypeEnum::Directional ? Vec4(1, 0, 0, 0) : Vec4(lc.attenuation, 0);
			uni.parameters[0] = cos(lc.spotAngle * 0.5);
			uni.parameters[1] = lc.spotExponent;
			return uni;
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
			Color,
		};

		enum class PrepareModeEnum
		{
			Shadowmap,
			Camera,
		};

		struct RenderPipelineImpl : public RenderPipeline, public RenderPipelineCreateConfig
		{
			Holder<Model> modelSquare, modelBone;
			Holder<ShaderProgram> shaderBlit, shaderDepth, shaderStandard, shaderDepthCutOut, shaderStandardCutOut;
			Holder<ShaderProgram> shaderVisualizeColor, shaderVisualizeDepth, shaderVisualizeMonochromatic;
			Holder<ShaderProgram> shaderFont;

			Holder<SkeletalAnimationPreparatorCollection> skeletonPreparatorCollection;
			EntityComponent *transformComponent = nullptr;
			EntityComponent *prevTransformComponent = nullptr;
			bool cnfRenderMissingModels = false;
			bool cnfRenderSkeletonBones = false;

			RenderPipelineImpl(const RenderPipelineCreateConfig &config) : RenderPipelineCreateConfig(config)
			{}

			static Holder<ShaderProgram> defaultProgram(const Holder<MultiShaderProgram> &multi, uint32 variant = 0)
			{
				if (multi)
					return multi->get(variant);
				return {};
			}

			bool reinitialize()
			{
				if (!assets->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/cage.pack")) || !assets->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/shader/engine/engine.pack")))
					return false;

				modelSquare = assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
				modelBone = assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/bone.obj"));
				shaderBlit = defaultProgram(assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/engine/blit.glsl")));
				Holder<MultiShaderProgram> standard = assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/engine/standard.glsl"));
				shaderStandard = defaultProgram(standard, 0);
				shaderStandardCutOut = defaultProgram(standard, HashString("CutOut"));
				shaderDepth = defaultProgram(standard, HashString("DepthOnly"));
				shaderDepthCutOut = defaultProgram(standard, HashString("DepthOnly") + HashString("CutOut"));
				shaderVisualizeColor = defaultProgram(assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/visualize/color.glsl")));
				shaderVisualizeDepth = defaultProgram(assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/visualize/depth.glsl")));
				shaderVisualizeMonochromatic = defaultProgram(assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/visualize/monochromatic.glsl")));
				shaderFont = defaultProgram(assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/gui/font.glsl")));
				CAGE_ASSERT(shaderBlit);

				skeletonPreparatorCollection = newSkeletalAnimationPreparatorCollection(assets, confRenderSkeletonBones);
				transformComponent = scene->component<TransformComponent>();
				prevTransformComponent = scene->componentsByType(detail::typeIndex<TransformComponent>())[1];
				cnfRenderMissingModels = confRenderMissingModels;
				cnfRenderSkeletonBones = confRenderSkeletonBones;

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
			void renderModelsImpl(const CameraData &data, const ModelShared &sh, const PointerRange<const UniMesh> uniMeshes, const PointerRange<const Mat3x4> uniArmatures, bool translucent) const
			{
				const Holder<RenderQueue> &renderQueue = data.renderQueue;

				Holder<ShaderProgram> shader = [&]() {
					if (sh.mesh->shaderName)
					{
						uint32 variant = 0;
						if constexpr (RenderMode != RenderModeEnum::Color)
							variant += HashString("DepthOnly");
						if (any(sh.mesh->flags & MeshRenderFlags::CutOut))
							variant += HashString("CutOut");
						return assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(sh.mesh->shaderName)->get(variant);
					}
					else
					{
						if constexpr (RenderMode == RenderModeEnum::Color)
						{ // color
							if (any(sh.mesh->flags & MeshRenderFlags::CutOut))
								return shaderStandardCutOut.share();
							else
								return shaderStandard.share();
						}
						else
						{ // depth
							if (any(sh.mesh->flags & MeshRenderFlags::CutOut))
								return shaderDepthCutOut.share();
							else
								return shaderDepth.share();
						}
					}
				}();
				renderQueue->bind(shader);

				renderQueue->culling(!any(sh.mesh->flags & MeshRenderFlags::TwoSided));
				renderQueue->depthTest(any(sh.mesh->flags & MeshRenderFlags::DepthTest));
				if constexpr (RenderMode == RenderModeEnum::Color)
				{
					renderQueue->depthWrite(any(sh.mesh->flags & MeshRenderFlags::DepthWrite));
					renderQueue->depthFuncLessEqual();
					renderQueue->blending(translucent);
					if (translucent)
						renderQueue->blendFuncPremultipliedTransparency();
				}
				else
				{
					renderQueue->depthWrite(true);
					renderQueue->depthFuncLess();
					renderQueue->blending(false);
				}

				std::array<uint32, CAGE_SHADER_MAX_ROUTINES> shaderRoutines = {};
				renderQueue->uniform(shader, CAGE_SHADER_UNI_LIGHTSCOUNT, data.lightsCount);
				renderQueue->uniform(shader, CAGE_SHADER_UNI_BONESPERINSTANCE, sh.mesh->bones);
				shaderRoutines[CAGE_SHADER_ROUTINEUNIF_SKELETON] = sh.skeletal ? 1 : 0;
				const bool ssao = !translucent && any(sh.mesh->flags & MeshRenderFlags::DepthWrite) && any(data.effects.effects & ScreenSpaceEffectsFlags::AmbientOcclusion);
				shaderRoutines[CAGE_SHADER_ROUTINEUNIF_AMBIENTOCCLUSION] = ssao ? 1 : 0;

				std::array<Holder<Texture>, MaxTexturesCountPerMaterial> textures = {};
				for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
				{
					const uint32 n = sh.mesh->textureNames[i];
					textures[i] = assets->tryGet<AssetSchemeIndexTexture, Texture>(n);
					if (textures[i])
					{
						switch (textures[i]->target())
						{
						case GL_TEXTURE_2D_ARRAY: renderQueue->bind(textures[i], CAGE_SHADER_TEXTURE_ALBEDO_ARRAY + i); break;
						case GL_TEXTURE_CUBE_MAP: renderQueue->bind(textures[i], CAGE_SHADER_TEXTURE_ALBEDO_CUBE + i); break;
						default: renderQueue->bind(textures[i], CAGE_SHADER_TEXTURE_ALBEDO + i); break;
						}
					}
				}
				updateShaderRoutinesForTextures(textures, shaderRoutines);

				renderQueue->uniform(shader, CAGE_SHADER_UNI_ROUTINES, shaderRoutines);

				const uint32 limit = sh.skeletal ? min(uint32(CAGE_SHADER_MAX_MESHES), CAGE_SHADER_MAX_BONES / sh.mesh->bones) : CAGE_SHADER_MAX_MESHES;
				for (uint32 offset = 0; offset < uniMeshes.size(); offset += limit)
				{
					const uint32 count = min(limit, numeric_cast<uint32>(uniMeshes.size()) - offset);
					renderQueue->universalUniformArray<const UniMesh>(subRange<const UniMesh>(uniMeshes, offset, count), CAGE_SHADER_UNIBLOCK_MESHES);
					if (sh.skeletal)
						renderQueue->universalUniformArray<const Mat3x4>(subRange<const Mat3x4>(uniArmatures, offset * sh.mesh->bones, count * sh.mesh->bones), CAGE_SHADER_UNIBLOCK_ARMATURES);
					renderQueue->draw(sh.mesh, count);
				}

				renderQueue->checkGlErrorDebug();
			}

			void renderTextImpl(const CameraData &data, const TextPrepare &text) const
			{
				const Holder<RenderQueue> &renderQueue = data.renderQueue;
				renderQueue->uniform(shaderFont, 0, data.viewProj * text.model);
				renderQueue->uniform(shaderFont, 4, text.color);
				text.font->render(+renderQueue, modelSquare, shaderFont, text.glyphs, text.format);
			}

			template<RenderModeEnum RenderMode>
			void renderLayer(const CameraData &data, const DataLayer &layer) const
			{
				const Holder<RenderQueue> &renderQueue = data.renderQueue;

				{
					const auto graphicsDebugScope = renderQueue->namedScope("opaque");
					for (const auto &shit : layer.opaque)
					{
						const ModelShared &sh = shit.first;
						if constexpr (RenderMode == RenderModeEnum::DepthPrepass)
						{
							if (none(sh.mesh->flags & MeshRenderFlags::DepthWrite))
								continue;
						}
						std::vector<UniMesh> uniMeshes;
						uniMeshes.reserve(shit.second.size());
						for (const ModelInstance &inst : shit.second)
							uniMeshes.push_back(inst.uni);
						std::vector<Mat3x4> uniArmatures;
						if (sh.skeletal)
						{
							uniArmatures.reserve(shit.second.size() * sh.mesh->bones);
							for (const ModelInstance &inst : shit.second)
							{
								const auto armature = inst.skeletalAnimation->armature();
								CAGE_ASSERT(armature.size() == sh.mesh->bones);
								for (const auto &it : armature)
									uniArmatures.push_back(it);
							}
						}
						renderModelsImpl<RenderMode>(data, sh, uniMeshes, uniArmatures, false);
					}
					renderQueue->resetAllState();
					renderQueue->viewport(Vec2i(), data.resolution);
				}

				if constexpr (RenderMode == RenderModeEnum::Color)
				{
					const auto graphicsDebugScope = renderQueue->namedScope("translucent");
					for (const auto &it : layer.translucent)
					{
						PointerRange<const UniMesh> uniMeshes = { &it.uni, &it.uni + 1 };
						PointerRange<const Mat3x4> uniArmatures;
						if (it.skeletal)
						{
							uniArmatures = it.skeletalAnimation->armature();
							CAGE_ASSERT(uniArmatures.size() == it.mesh->bones);
						}
						renderModelsImpl<RenderMode>(data, it, uniMeshes, uniArmatures, true);
					}
					renderQueue->resetAllState();
					renderQueue->viewport(Vec2i(), data.resolution);
				}

				if constexpr (RenderMode == RenderModeEnum::Color)
				{
					const auto graphicsDebugScope = renderQueue->namedScope("texts");
					renderQueue->depthTest(true);
					renderQueue->depthWrite(false);
					renderQueue->culling(true);
					renderQueue->blending(true);
					renderQueue->blendFuncAlphaTransparency();
					for (const TextPrepare &text : layer.texts)
						renderTextImpl(data, text);
					renderQueue->resetAllState();
					renderQueue->viewport(Vec2i(), data.resolution);
				}

				renderQueue->checkGlErrorDebug();
			}

			template<RenderModeEnum RenderMode>
			void renderModels(const CameraData &data) const
			{
				for (const auto &it : data.layers)
					renderLayer<RenderMode>(data, it.second);
			}

			template<PrepareModeEnum PrepareMode>
			void prepareModelImpl(CameraData &data, ModelPrepare &prepare) const
			{
				DataLayer &layer = data.layers[prepare.mesh->layer];
				if (PrepareMode == PrepareModeEnum::Camera && prepare.translucent)
				{
					ModelTranslucent &tr = (ModelTranslucent &)prepare;
					layer.translucent.push_back(std::move(tr));
				}
				else
				{
					ModelShared &sh = (ModelShared &)prepare;
					ModelInstance &inst = (ModelInstance &)prepare;
					layer.opaque[std::move(sh)].push_back(std::move(inst));
				}
			}

			template<PrepareModeEnum PrepareMode>
			void prepareModelBones(CameraData &data, const ModelPrepare &prepare) const
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
					pr.uni.normalMat.data[2][3] = prepare.uni.normalMat.data[2][3];
					pr.translucent = false;
					prepareModelImpl<PrepareMode>(data, pr);
				}
			}

			template<PrepareModeEnum PrepareMode>
			void prepareModel(CameraData &data, ModelPrepare &pr, Holder<RenderObject> parent = {}) const
			{
				if (!pr.mesh)
					return;

				if constexpr (PrepareMode == PrepareModeEnum::Shadowmap)
				{
					if (none(pr.mesh->flags & MeshRenderFlags::ShadowCast))
						return;
				}

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
					if (Holder<Texture> tex = assets->tryGet<AssetSchemeIndexTexture, Texture>(pr.mesh->textureNames[0]))
						pr.uni.aniTexFrames = detail::evalSamplesForTextureAnimation(+tex, currentTime, pt->startTime, pt->speed, pt->offset);
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
					Holder<SkeletalAnimation> anim = assets->tryGet<AssetSchemeIndexSkeletalAnimation, SkeletalAnimation>(ps->name);
					if (anim)
					{
						Real coefficient = detail::evalCoefficientForSkeletalAnimation(+anim, currentTime, ps->startTime, ps->speed, ps->offset);
						pr.skeletalAnimation = skeletonPreparatorCollection->create(pr.e, std::move(anim), coefficient);
						pr.skeletalAnimation->prepare();
						pr.skeletal = true;
					}
				}
				CAGE_ASSERT(!!pr.skeletal == !!pr.skeletalAnimation);

				pr.translucent = any(pr.mesh->flags & (MeshRenderFlags::Transparent | MeshRenderFlags::Fade)) || pr.render.opacity < 1;
				pr.uni.normalMat.data[2][3] = any(pr.mesh->flags & MeshRenderFlags::Lighting) ? 1 : 0; // is lighting enabled
				pr.uni.normalMat.data[1][3] = any(pr.mesh->flags & MeshRenderFlags::Fade) ? 1 : 0; // transparent mode is to fade

				pr.depth = (pr.uni.mvpMat * Vec4(0, 0, 0, 1))[2];

				if (pr.skeletal && cnfRenderSkeletonBones)
					prepareModelBones<PrepareMode>(data, pr);
				else
					prepareModelImpl<PrepareMode>(data, pr);
			}

			template<PrepareModeEnum PrepareMode>
			void prepareObject(CameraData &data, const ModelPrepare &prepare, Holder<RenderObject> object) const
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
					pr.mesh = assets->tryGet<AssetSchemeIndexModel, Model>(it);
					prepareModel<PrepareMode>(data, pr, object.share());
				}
			}

			template<PrepareModeEnum PrepareMode>
			void prepareEntities(CameraData &data) const
			{
				entitiesVisitor([&](Entity *e, const RenderComponent &rc) {
					if ((rc.sceneMask & data.camera.sceneMask) == 0)
						return;
					ModelPrepare prepare;
					prepare.e = e;
					prepare.render = rc;
					prepare.model = modelTransform(e);
					prepare.uni = initializeMeshUni(data, prepare.model);
					prepare.frustum = Frustum(prepare.uni.mvpMat);
					if (Holder<RenderObject> obj = assets->tryGet<AssetSchemeIndexRenderObject, RenderObject>(rc.object))
					{
						prepareObject<PrepareMode>(data, prepare, std::move(obj));
						return;
					}
					if (Holder<Model> mesh = assets->tryGet<AssetSchemeIndexModel, Model>(rc.object))
					{
						prepare.mesh = std::move(mesh);
						prepareModel<PrepareMode>(data, prepare);
						return;
					}
					if (cnfRenderMissingModels)
					{
						prepare.mesh = assets->tryGet<AssetSchemeIndexModel, Model>(HashString("cage/model/fake.obj"));
						prepareModel<PrepareMode>(data, prepare);
						return;
					}
				}, +scene, false);

				if constexpr (PrepareMode == PrepareModeEnum::Camera)
				{
					// sort translucent back-to-front
					for (auto &it : data.layers)
						std::sort(it.second.translucent.begin(), it.second.translucent.end(), [](const auto &a, const auto &b) { return a.depth > b.depth; });

					entitiesVisitor([&](Entity *e, const TextComponent &tc_) {
						if ((tc_.sceneMask & data.camera.sceneMask) == 0)
							return;
						TextComponent pt = tc_;
						TextPrepare prepare;
						if (!pt.font)
							pt.font = HashString("cage/font/ubuntu/Ubuntu-R.ttf");
						prepare.font = assets->tryGet<AssetSchemeIndexFont, Font>(pt.font);
						if (!prepare.font)
							return;
						const String str = loadFormattedString(assets, pt.assetName, pt.textName, pt.value);
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
						data.layers[0].texts.push_back(std::move(prepare));
					}, +scene, false);
				}
			}

			void taskShadowmap(ShadowmapData &data, uint32) const
			{
				data.renderQueue = newRenderQueue(data.name, provisionalGraphics);
				Holder<RenderQueue> &renderQueue = data.renderQueue;
				const auto graphicsDebugScope = renderQueue->namedScope("shadowmap");

				FrameBufferHandle renderTarget = provisionalGraphics->frameBufferDraw("renderTarget");
				renderQueue->bind(renderTarget);
				renderQueue->clearFrameBuffer(renderTarget);
				renderQueue->depthTexture(renderTarget, data.shadowTexture);
				renderQueue->activeAttachments(renderTarget, 0);
				renderQueue->checkFrameBuffer(renderTarget);
				renderQueue->viewport(Vec2i(), data.resolution);
				renderQueue->colorWrite(false);
				renderQueue->clear(false, true);
				renderQueue->bind(shaderDepth);
				renderQueue->checkGlErrorDebug();

				prepareEntities<PrepareModeEnum::Shadowmap>(data);
				renderModels<RenderModeEnum::Shadowmap>(data);

				renderQueue->resetFrameBuffer();
				renderQueue->resetAllState();
				renderQueue->resetAllTextures();
				renderQueue->checkGlErrorDebug();

				RenderPipelineDebugVisualization deb;
				deb.texture = data.shadowTexture.share();
				deb.shader = shaderVisualizeDepth.share();
				data.debugVisualizations.push_back(std::move(deb));
			}

			void prepareCameraLights(CameraData &data) const
			{
				const Holder<RenderQueue> &renderQueue = data.renderQueue;

				std::vector<UniLight> lights;
				lights.reserve(50);
				std::vector<Mat4> shadows;
				PointerRangeHolder<TextureHandle> tex2d, texCube;

				// add shadowed lights
				for (const auto &[e, sh] : data.shadowmaps)
				{
					lights.push_back(sh.shadowUni);
					shadows.push_back(sh.shadowMat);
					if (sh.lightComponent.lightType == LightTypeEnum::Spot)
					{
						tex2d.push_back({});
						texCube.push_back(sh.shadowTexture);
					}
					else
					{
						tex2d.push_back(sh.shadowTexture);
						texCube.push_back({});
					}
				}

				// add unshadowed lights
				entitiesVisitor([&](Entity *e, const LightComponent &lc) {
					if ((lc.sceneMask & data.camera.sceneMask) == 0)
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
				}, +scene, false);

				data.lightsCount = numeric_cast<uint32>(lights.size());
				if (!lights.empty())
					renderQueue->universalUniformArray<UniLight>(lights, CAGE_SHADER_UNIBLOCK_LIGHTS);
				if (!shadows.empty())
				{
					renderQueue->universalUniformArray<Mat4>(shadows, CAGE_SHADER_UNIBLOCK_SHADOWSMATRICES);
					if (!tex2d.empty())
						renderQueue->bindlessUniform(std::move(tex2d), CAGE_SHADER_UNIBLOCK_SHADOWS2D, true);
					if (!texCube.empty())
						renderQueue->bindlessUniform(std::move(texCube), CAGE_SHADER_UNIBLOCK_SHADOWSCUBE, true);
				}
			}

			void finishCameraLights(CameraData &data) const
			{
				PointerRangeHolder<TextureHandle> texs;
				for (const auto &[e, sh] : data.shadowmaps)
					texs.push_back(sh.shadowTexture);
				data.renderQueue->bindlessResident(std::move(texs), false);
			}

			void taskCamera(CameraData &data, uint32) const
			{
				data.renderQueue = newRenderQueue(data.name + "_camera", provisionalGraphics);
				Holder<RenderQueue> &renderQueue = data.renderQueue;
				const auto graphicsDebugScope = renderQueue->namedScope("camera");

				if (confNoAmbientOcclusion)
					data.effects.effects &= ~ScreenSpaceEffectsFlags::AmbientOcclusion;
				if (confNoBloom)
					data.effects.effects &= ~ScreenSpaceEffectsFlags::Bloom;

				data.model = Mat4(data.transform);
				data.view = inverse(data.model);
				data.viewProj = data.projection * data.view;
				prepareEntities<PrepareModeEnum::Camera>(data);
				prepareCameraLights(data);

				TextureHandle colorTexture = [&]() {
					TextureHandle t = provisionalGraphics->texture(Stringizer() + "colorTarget_" + data.name + "_" + data.resolution, [resolution = data.resolution](Texture *t) {
						t->initialize(resolution, 1, GL_RGBA16F);
						t->filters(GL_LINEAR, GL_LINEAR, 0);
						t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					});
					RenderPipelineDebugVisualization deb;
					deb.texture = t;
					deb.shader = shaderVisualizeColor.share();
					data.debugVisualizations.push_back(std::move(deb));
					return t;
				}();
				TextureHandle depthTexture = [&]() {
					TextureHandle t = provisionalGraphics->texture(Stringizer() + "depthTarget_" + data.name + "_" + data.resolution, [resolution = data.resolution](Texture *t) {
						t->initialize(resolution, 1, GL_DEPTH_COMPONENT32);
						t->filters(GL_LINEAR, GL_LINEAR, 0);
						t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					});
					RenderPipelineDebugVisualization deb;
					deb.texture = t;
					deb.shader = shaderVisualizeDepth.share();
					data.debugVisualizations.push_back(std::move(deb));
					return t;
				}();
				FrameBufferHandle renderTarget = provisionalGraphics->frameBufferDraw("renderTarget");
				renderQueue->bind(renderTarget);
				renderQueue->depthTexture(renderTarget, depthTexture);
				renderQueue->colorTexture(renderTarget, 0, colorTexture);
				renderQueue->activeAttachments(renderTarget, 1);
				renderQueue->checkFrameBuffer(renderTarget);
				renderQueue->checkGlErrorDebug();
				renderQueue->bind(depthTexture, CAGE_SHADER_TEXTURE_DEPTH);

				renderQueue->viewport(Vec2i(), data.resolution);
				renderQueue->clear(true, true);

				ScreenSpaceCommonConfig commonConfig; // helper to simplify initialization
				commonConfig.assets = assets;
				commonConfig.provisionals = +provisionalGraphics;
				commonConfig.queue = +renderQueue;
				commonConfig.resolution = data.resolution;

				{
					const auto graphicsDebugScope = renderQueue->namedScope("depth prepass");
					renderModels<RenderModeEnum::DepthPrepass>(data);
				}

				if (any(data.effects.effects & ScreenSpaceEffectsFlags::AmbientOcclusion))
				{
					const Vec2i ssaoResolution = data.resolution / 3;

					TextureHandle depthTextureLowRes = [&]() {
						const auto graphicsDebugScope = renderQueue->namedScope("lowResDepth");
						TextureHandle t = provisionalGraphics->texture(Stringizer() + "depthTextureLowRes_" + data.name + "_" + data.resolution, [ssaoResolution](Texture *t) {
							t->initialize(ssaoResolution, 1, GL_R32F);
							t->filters(GL_NEAREST, GL_NEAREST, 0);
							t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
						});
						RenderPipelineDebugVisualization deb;
						deb.texture = t;
						deb.shader = shaderVisualizeDepth.share();
						data.debugVisualizations.push_back(std::move(deb));

						FrameBufferHandle renderTarget = provisionalGraphics->frameBufferDraw(Stringizer() + "depthTargetLowRes_" + data.name);
						renderQueue->bind(renderTarget);
						renderQueue->colorTexture(renderTarget, 0, t);
						renderQueue->depthTexture(renderTarget, {});
						renderQueue->activeAttachments(renderTarget, 1);
						renderQueue->checkFrameBuffer(renderTarget);
						renderQueue->viewport(Vec2i(), ssaoResolution);
						renderQueue->bind(shaderVisualizeColor);
						renderQueue->uniform(shaderVisualizeColor, 0, 1 / Vec2(ssaoResolution));
						renderQueue->bind(depthTexture, 0);
						renderQueue->draw(modelSquare);
						renderQueue->resetAllState();
						renderQueue->resetFrameBuffer();
						return t;
					}();

					ScreenSpaceAmbientOcclusionConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					(ScreenSpaceAmbientOcclusion &)cfg = data.effects.ssao;
					cfg.resolution = ssaoResolution;
					cfg.proj = data.projection;
					cfg.inDepth = depthTextureLowRes;
					cfg.frameIndex = frameIndex;
					screenSpaceAmbientOcclusion(cfg);

					{
						RenderPipelineDebugVisualization deb;
						deb.texture = cfg.outAo;
						deb.shader = shaderVisualizeMonochromatic.share();
						data.debugVisualizations.push_back(std::move(deb));
					}

					// bind the texture for sampling
					renderQueue->bind(cfg.outAo, CAGE_SHADER_TEXTURE_SSAO);

					// restore rendering state
					renderQueue->bind(renderTarget);
					renderQueue->viewport(Vec2i(), data.resolution);
				}

				{
					const auto graphicsDebugScope = renderQueue->namedScope("standard");
					renderModels<RenderModeEnum::Color>(data);
				}

				finishCameraLights(data);

				{
					const auto graphicsDebugScope = renderQueue->namedScope("effects");

					TextureHandle texSource = colorTexture;
					TextureHandle texTarget = [&]() {
						TextureHandle t = provisionalGraphics->texture(Stringizer() + "intermediateTarget_" + data.resolution, [resolution = data.resolution](Texture *t) {
							t->initialize(resolution, 1, GL_RGBA16F);
							t->filters(GL_LINEAR, GL_LINEAR, 0);
							t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
						});
						return t;
					}();

					// depth of field
					if (any(data.effects.effects & ScreenSpaceEffectsFlags::DepthOfField))
					{
						ScreenSpaceDepthOfFieldConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						(ScreenSpaceDepthOfField &)cfg = data.effects.depthOfField;
						cfg.proj = data.projection;
						cfg.inDepth = depthTexture;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						screenSpaceDepthOfField(cfg);
						std::swap(texSource, texTarget);
					}

					// eye adaptation
					if (any(data.effects.effects & ScreenSpaceEffectsFlags::EyeAdaptation))
					{
						ScreenSpaceEyeAdaptationConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						(ScreenSpaceEyeAdaptation &)cfg = data.effects.eyeAdaptation;
						cfg.cameraId = data.name;
						cfg.inColor = texSource;
						cfg.elapsedTime = elapsedTime * 1e-6;
						screenSpaceEyeAdaptationPrepare(cfg);
					}

					// bloom
					if (any(data.effects.effects & ScreenSpaceEffectsFlags::Bloom))
					{
						ScreenSpaceBloomConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						(ScreenSpaceBloom &)cfg = data.effects.bloom;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						screenSpaceBloom(cfg);
						std::swap(texSource, texTarget);
					}

					// eye adaptation
					if (any(data.effects.effects & ScreenSpaceEffectsFlags::EyeAdaptation))
					{
						ScreenSpaceEyeAdaptationConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						(ScreenSpaceEyeAdaptation &)cfg = data.effects.eyeAdaptation;
						cfg.cameraId = data.name;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						cfg.elapsedTime = elapsedTime * 1e-6;
						screenSpaceEyeAdaptationApply(cfg);
						std::swap(texSource, texTarget);
					}

					// final screen effects
					if (any(data.effects.effects & (ScreenSpaceEffectsFlags::ToneMapping | ScreenSpaceEffectsFlags::GammaCorrection)))
					{
						ScreenSpaceTonemapConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						(ScreenSpaceTonemap &)cfg = data.effects.tonemap;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						cfg.tonemapEnabled = any(data.effects.effects & ScreenSpaceEffectsFlags::ToneMapping);
						cfg.gamma = any(data.effects.effects & ScreenSpaceEffectsFlags::GammaCorrection) ? data.effects.gamma : 1;
						screenSpaceTonemap(cfg);
						std::swap(texSource, texTarget);
					}

					// fxaa
					if (any(data.effects.effects & ScreenSpaceEffectsFlags::AntiAliasing))
					{
						ScreenSpaceFastApproximateAntiAliasingConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						screenSpaceFastApproximateAntiAliasing(cfg);
						std::swap(texSource, texTarget);
					}

					// blit to destination
					if (texSource != colorTexture)
					{
						renderQueue->viewport(Vec2i(), data.resolution);
						renderQueue->bind(texSource, 0);
						renderQueue->bind(shaderBlit);
						renderQueue->bind(renderTarget);
						renderQueue->colorTexture(renderTarget, 0, colorTexture);
						renderQueue->activeAttachments(renderTarget, 1);
						renderQueue->checkFrameBuffer(renderTarget);
						renderQueue->draw(modelSquare);
					}

					renderQueue->checkGlErrorDebug();
				}

				{
					const auto graphicsDebugScope = renderQueue->namedScope("final blit");
					renderQueue->resetAllState();
					renderQueue->viewport(Vec2i(), data.resolution);
					renderQueue->bind(colorTexture, 0);
					renderQueue->bind(shaderBlit);
					if (data.target)
					{ // blit to texture
						renderQueue->bind(renderTarget);
						renderQueue->colorTexture(renderTarget, 0, data.target);
						renderQueue->depthTexture(renderTarget, {});
						renderQueue->activeAttachments(renderTarget, 1);
						renderQueue->checkFrameBuffer(renderTarget);
						renderQueue->draw(modelSquare);
						renderQueue->resetFrameBuffer();
					}
					else
					{ // blit to window
						renderQueue->resetFrameBuffer();
						renderQueue->draw(modelSquare);
					}
					renderQueue->resetAllState();
					renderQueue->resetAllTextures();
					renderQueue->checkGlErrorDebug();
				}
			}

			Holder<AsyncTask> prepareShadowmap(CameraData &camera, Entity *e, const LightComponent &lc, const ShadowmapComponent &sc) const
			{
				ShadowmapData &data = camera.shadowmaps[e];
				data.name = Stringizer() + camera.name + "_shadowmap_" + e->name();
				data.camera.sceneMask = camera.camera.sceneMask;
				data.lightComponent = lc;
				data.shadowmapComponent = sc;
				data.resolution = Vec2i(data.shadowmapComponent.resolution);
				data.model = modelTransform(e);
				data.view = inverse(data.model);
				data.projection = [&]() {
					const auto &sc = data.shadowmapComponent;
					switch (data.lightComponent.lightType)
					{
					case LightTypeEnum::Directional: return orthographicProjection(-sc.worldSize[0], sc.worldSize[0], -sc.worldSize[1], sc.worldSize[1], -sc.worldSize[2], sc.worldSize[2]);
					case LightTypeEnum::Spot: return perspectiveProjection(data.lightComponent.spotAngle, 1, sc.worldSize[0], sc.worldSize[1]);
					case LightTypeEnum::Point: return perspectiveProjection(Degs(90), 1, sc.worldSize[0], sc.worldSize[1]);
					default: CAGE_THROW_CRITICAL(Exception, "invalid light type");
					}
				}();
				data.viewProj = data.projection * data.view;
				data.lodSelection = camera.lodSelection;
				static constexpr Mat4 bias = Mat4(
					0.5, 0.0, 0.0, 0.0,
					0.0, 0.5, 0.0, 0.0,
					0.0, 0.0, 0.5, 0.0,
					0.5, 0.5, 0.5, 1.0);
				data.shadowMat = bias * data.viewProj;

				{
					const String name = Stringizer() + data.name + "_" + data.resolution;
					data.shadowTexture = provisionalGraphics->texture(name, data.lightComponent.lightType == LightTypeEnum::Point ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, [resolution = data.resolution, format = data.lightComponent.lightType == LightTypeEnum::Point ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT24](Texture *t) {
						t->initialize(resolution, 1, format);
						t->filters(GL_LINEAR, GL_LINEAR, 16);
						t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					});
				}

				{
					UniLight &uni = data.shadowUni;
					uni = initializeLightUni(data.model, data.lightComponent);
					uni.parameters[2] = e->value<ShadowmapComponent>().normalOffsetScale;
					uni.parameters[3] = [&]() {
						switch (data.lightComponent.lightType)
						{
						case LightTypeEnum::Directional: return CAGE_SHADER_ROUTINEPROC_LIGHTDIRECTIONALSHADOW;
						case LightTypeEnum::Spot: return CAGE_SHADER_ROUTINEPROC_LIGHTSPOTSHADOW;
						case LightTypeEnum::Point: return CAGE_SHADER_ROUTINEPROC_LIGHTPOINTSHADOW;
						default: CAGE_THROW_CRITICAL(Exception, "invalid light type");
						}
					}();
				}

				return tasksRunAsync<ShadowmapData>("render shadowmap task", Delegate<void(ShadowmapData&, uint32)>().bind<RenderPipelineImpl, &RenderPipelineImpl::taskShadowmap>(this), Holder<ShadowmapData>(&data, nullptr), 1, tasksCurrentPriority() + 9);
			}

			RenderPipelineResult prepareCamera(const RenderPipelineCamera &camera) const
			{
				CAGE_ASSERT(!camera.name.empty());

				CameraData data;
				(RenderPipelineCamera &)data = camera;

				std::vector<Holder<AsyncTask>> tasks;
				entitiesVisitor([&](Entity *e, const LightComponent &lc, const ShadowmapComponent &sc) {
					if ((lc.sceneMask & data.camera.sceneMask) == 0)
						return;
					tasks.push_back(prepareShadowmap(data, e, lc, sc));
				}, +scene, false);
				tasks.push_back(tasksRunAsync<CameraData>("render camera task", Delegate<void(CameraData&, uint32)>().bind<RenderPipelineImpl, &RenderPipelineImpl::taskCamera>(this), Holder<CameraData>(&data, nullptr), 1, tasksCurrentPriority() + 10));
				for (const auto &it : tasks)
					it->wait();

				Holder<RenderQueue> queue = newRenderQueue(camera.name + "_pipeline", provisionalGraphics);

				{
					// viewport must be dispatched before shadowmaps
					UniViewport viewport;
					viewport.vMat = data.view;
					viewport.pMat = data.projection;
					viewport.vpMat = data.viewProj;
					viewport.vpInv = inverse(data.viewProj);
					viewport.eyePos = data.model * Vec4(0, 0, 0, 1);
					viewport.eyeDir = data.model * Vec4(0, 0, -1, 0);
					viewport.ambientLight = Vec4(colorGammaToLinear(data.camera.ambientColor) * data.camera.ambientIntensity, 0);
					viewport.ambientDirectionalLight = Vec4(colorGammaToLinear(data.camera.ambientDirectionalColor) * data.camera.ambientDirectionalIntensity, 0);
					viewport.viewport = Vec4(Vec2(), Vec2(data.resolution));
					viewport.time = Vec4(frameIndex % 10000, (currentTime % uint64(1e6)) / 1e6, (currentTime % uint64(1e9)) / 1e9, 0);
					queue->universalUniformStruct(viewport, CAGE_SHADER_UNIBLOCK_VIEWPORT);
				}

				// ensure that shadowmaps are rendered before the camera
				for (auto &shm : data.shadowmaps)
				{
					queue->enqueue(std::move(shm.second.renderQueue));
					for (RenderPipelineDebugVisualization &di : shm.second.debugVisualizations)
						data.debugVisualizations.push_back(std::move(di));
				}

				queue->enqueue(std::move(data.renderQueue));

				RenderPipelineResult result;
				result.debugVisualizations = std::move(data.debugVisualizations);
				result.renderQueue = std::move(queue);
				return result;
			}
		};
	}

	bool RenderPipeline::reinitialize()
	{
		RenderPipelineImpl *impl = (RenderPipelineImpl *)this;
		return impl->reinitialize();
	}

	RenderPipelineResult RenderPipeline::render(const RenderPipelineCamera &camera)
	{
		RenderPipelineImpl *impl = (RenderPipelineImpl *)this;
		return impl->prepareCamera(camera);
	}

	Holder<RenderPipeline> newRenderPipeline(const RenderPipelineCreateConfig &config)
	{
		return systemMemory().createImpl<RenderPipeline, RenderPipelineImpl>(config);
	}
}
