#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <vector>

#include <cage-core/assetsManager.h>
#include <cage-core/assetsOnDemand.h>
#include <cage-core/camera.h>
#include <cage-core/color.h>
#include <cage-core/config.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/geometry.h>
#include <cage-core/hashString.h>
#include <cage-core/meshIoCommon.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/profiling.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/skeletalAnimationPreparator.h>
#include <cage-core/tasks.h>
#include <cage-core/texts.h>
#include <cage-engine/font.h>
#include <cage-engine/model.h>
#include <cage-engine/opengl.h> // all the constants
#include <cage-engine/provisionalGraphics.h>
#include <cage-engine/provisionalHandles.h>
#include <cage-engine/renderObject.h>
#include <cage-engine/renderPipeline.h>
#include <cage-engine/renderQueue.h>
#include <cage-engine/scene.h>
#include <cage-engine/screenSpaceEffects.h>
#include <cage-engine/shaderConventions.h>
#include <cage-engine/shaderProgram.h>
#include <cage-engine/texture.h>
#include <cage-engine/window.h>

namespace cage
{
	namespace
	{
		const ConfigBool confRenderMissingModels("cage/graphics/renderMissingModels", false);
		const ConfigBool confRenderSkeletonBones("cage/graphics/renderSkeletonBones", false);

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
			Vec4 time; // frame index (loops at 10000), time (loops every second), time (loops every 1000 seconds)
		};

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
			Vec4 color; // linear RGB, intensity
			Vec4 position; // xyz, maxDistance
			Vec4 direction; // xyz, unused
			Vec4 attenuation; // attenuationType, minDistance, maxDistance, unused
			Vec4 params; // lightType, ssaoFactor, spotAngle, spotExponent
		};

		struct UniShadowedLight : public UniLight
		{
			Mat4 shadowMat;
			Vec4 shadowParams; // shadowmapSamplerIndex, shadowedLightIndex (unused?), normalOffsetScale, shadowFactor
		};

		struct UniOptions
		{
			Vec4i options[(CAGE_SHADER_MAX_OPTIONS + 3) / 4];

			sint32 &operator[](uint32 index) { return options[index / 4][index % 4]; }
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

			bool operator<(const ModelShared &other) const { return cmp() < other.cmp(); }
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
		struct CameraData
		{
			String name;
			ScreenSpaceEffectsComponent effects;
			Mat4 projection;
			Transform transform;
			TextureHandle target;
			CameraCommonProperties camera;
			LodSelection lodSelection;
			Vec2i resolution;

			Mat4 model;
			Mat4 view;
			Mat4 viewProj;

			std::map<sint32, DataLayer> layers;
			Holder<RenderQueue> renderQueue;

			std::map<Entity *, struct ShadowmapData> shadowmaps;
			uint32 lightsCount = 0;
			uint32 shadowedLightsCount = 0;
		};

		struct ShadowmapData : public CameraData
		{
			UniShadowedLight shadowUni;
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

		UniLight initializeLightUni(const Mat4 &model, const LightComponent &lc, const Vec3 &cameraCenter)
		{
			CAGE_ASSERT(lc.minDistance > 0 && lc.maxDistance > lc.minDistance);
			static constexpr Real RoundingOffset = 1e-7;
			UniLight uni;

			uni.color = Vec4(colorGammaToLinear(lc.color), lc.intensity);
			{
				// move as much of the energy from the intensity to the color
				// minimizing the intensity ensures tighter bounds on fragments that need lighting
				Real c = max(uni.color[0], max(uni.color[1], uni.color[2]));
				c = clamp(c, 0.01, 1);
				uni.color = Vec4(Vec3(uni.color) / c, uni.color[3] * c);
			}

			uni.position = model * Vec4(0, 0, 0, 1);
			uni.direction = normalize(model * Vec4(0, 0, -1, 0));

			if (lc.lightType != LightTypeEnum::Directional)
			{
				uni.attenuation[0] = (int)lc.attenuation + RoundingOffset;
				uni.attenuation[1] = lc.minDistance;
				uni.attenuation[2] = lc.maxDistance;
			}

			if (lc.lightType == LightTypeEnum::Point)
				uni.position[3] = lc.maxDistance / (distance(Vec3(uni.position), cameraCenter) + 1e-5);
			else
				uni.position[3] = Real::Infinity();

			uni.params[0] = [&]() -> Real
			{
				switch (lc.lightType)
				{
					case LightTypeEnum::Directional:
						return CAGE_SHADER_OPTIONVALUE_LIGHTDIRECTIONAL + RoundingOffset;
					case LightTypeEnum::Spot:
						return CAGE_SHADER_OPTIONVALUE_LIGHTSPOT + RoundingOffset;
					case LightTypeEnum::Point:
						return CAGE_SHADER_OPTIONVALUE_LIGHTPOINT + RoundingOffset;
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid light type");
				}
			}();
			uni.params[1] = lc.ssaoFactor;
			uni.params[2] = cos(lc.spotAngle * 0.5);
			uni.params[3] = lc.spotExponent;

			return uni;
		}

		void filterLightsOverLimit(std::vector<UniLight> &lights, uint32 limit)
		{
			std::sort(lights.begin(), lights.end(), [&](const UniLight &a, const UniLight &b) { return a.position[3] > b.position[3]; });
			limit = min(limit, (uint32)CAGE_SHADER_MAX_LIGHTS);
			if (lights.size() > limit)
				lights.resize(limit);

			// fade-out lights close to limit
			const uint32 s = max(limit * 85 / 100, 10u);
			const Real f = 1.0 / (limit - s + 1);
			for (uint32 i = s; i < lights.size(); i++)
				lights[i].color[3] *= saturate(1 - (i - s + 1) * f);
		}

		void updateShaderRoutinesForTextures(const std::array<Holder<Texture>, MaxTexturesCountPerMaterial> &textures, UniOptions &options)
		{
			if (textures[CAGE_SHADER_TEXTURE_ALBEDO])
			{
				switch (textures[CAGE_SHADER_TEXTURE_ALBEDO]->target())
				{
					case GL_TEXTURE_2D_ARRAY:
						options[CAGE_SHADER_OPTIONINDEX_MAPALBEDO] = CAGE_SHADER_OPTIONVALUE_MAPALBEDOARRAY;
						break;
					case GL_TEXTURE_CUBE_MAP:
						options[CAGE_SHADER_OPTIONINDEX_MAPALBEDO] = CAGE_SHADER_OPTIONVALUE_MAPALBEDOCUBE;
						break;
					default:
						options[CAGE_SHADER_OPTIONINDEX_MAPALBEDO] = CAGE_SHADER_OPTIONVALUE_MAPALBEDO2D;
						break;
				}
			}
			else
				options[CAGE_SHADER_OPTIONINDEX_MAPALBEDO] = 0;

			if (textures[CAGE_SHADER_TEXTURE_SPECIAL])
			{
				switch (textures[CAGE_SHADER_TEXTURE_SPECIAL]->target())
				{
					case GL_TEXTURE_2D_ARRAY:
						options[CAGE_SHADER_OPTIONINDEX_MAPSPECIAL] = CAGE_SHADER_OPTIONVALUE_MAPSPECIALARRAY;
						break;
					case GL_TEXTURE_CUBE_MAP:
						options[CAGE_SHADER_OPTIONINDEX_MAPSPECIAL] = CAGE_SHADER_OPTIONVALUE_MAPSPECIALCUBE;
						break;
					default:
						options[CAGE_SHADER_OPTIONINDEX_MAPSPECIAL] = CAGE_SHADER_OPTIONVALUE_MAPSPECIAL2D;
						break;
				}
			}
			else
				options[CAGE_SHADER_OPTIONINDEX_MAPSPECIAL] = 0;

			if (textures[CAGE_SHADER_TEXTURE_NORMAL])
			{
				switch (textures[CAGE_SHADER_TEXTURE_NORMAL]->target())
				{
					case GL_TEXTURE_2D_ARRAY:
						options[CAGE_SHADER_OPTIONINDEX_MAPNORMAL] = CAGE_SHADER_OPTIONVALUE_MAPNORMALARRAY;
						break;
					case GL_TEXTURE_CUBE_MAP:
						options[CAGE_SHADER_OPTIONINDEX_MAPNORMAL] = CAGE_SHADER_OPTIONVALUE_MAPNORMALCUBE;
						break;
					default:
						options[CAGE_SHADER_OPTIONINDEX_MAPNORMAL] = CAGE_SHADER_OPTIONVALUE_MAPNORMAL2D;
						break;
				}
			}
			else
				options[CAGE_SHADER_OPTIONINDEX_MAPNORMAL] = 0;
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

		struct RenderPipelineImpl : RenderPipelineConfig
		{
			Holder<Model> modelSquare, modelBone;
			Holder<ShaderProgram> shaderBlitPixels, shaderBlitScaled;
			Holder<ShaderProgram> shaderDepth, shaderStandard, shaderDepthCutOut, shaderStandardCutOut;
			Holder<ShaderProgram> shaderText;

			Holder<SkeletalAnimationPreparatorCollection> skeletonPreparatorCollection;
			EntityComponent *transformComponent = nullptr;
			EntityComponent *prevTransformComponent = nullptr;

			bool cnfRenderMissingModels = confRenderMissingModels;
			bool cnfRenderSkeletonBones = confRenderSkeletonBones;

			explicit RenderPipelineImpl(const RenderPipelineConfig &config) : RenderPipelineConfig(config)
			{
				if (!assets->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/cage.pack")) || !assets->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/shader/engine/engine.pack")))
					return;

				modelSquare = assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
				modelBone = assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/bone.obj"));
				CAGE_ASSERT(modelSquare && modelBone);
				shaderBlitPixels = assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/engine/blitPixels.glsl"))->get(0);
				shaderBlitScaled = assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/engine/blitScaled.glsl"))->get(0);
				CAGE_ASSERT(shaderBlitPixels && shaderBlitScaled);
				Holder<MultiShaderProgram> standard = assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/engine/standard.glsl"));
				CAGE_ASSERT(standard);
				shaderDepth = standard->get(HashString("DepthOnly"));
				shaderDepthCutOut = standard->get(HashString("DepthOnly") + HashString("CutOut"));
				shaderStandard = standard->get(0);
				shaderStandardCutOut = standard->get(HashString("CutOut"));
				shaderText = assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/engine/text.glsl"))->get(0);

				transformComponent = scene->component<TransformComponent>();
				prevTransformComponent = scene->componentsByType(detail::typeIndex<TransformComponent>())[1];

				skeletonPreparatorCollection = newSkeletalAnimationPreparatorCollection(assets);
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

				Holder<ShaderProgram> shader = [&]()
				{
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

				renderQueue->culling(none(sh.mesh->flags & MeshRenderFlags::TwoSided));
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

				UniOptions uniOptions;
				uniOptions[CAGE_SHADER_OPTIONINDEX_BONESCOUNT] = sh.mesh->bones;
				uniOptions[CAGE_SHADER_OPTIONINDEX_LIGHTSCOUNT] = data.lightsCount;
				uniOptions[CAGE_SHADER_OPTIONINDEX_SHADOWEDLIGHTSCOUNT] = data.shadowedLightsCount;
				uniOptions[CAGE_SHADER_OPTIONINDEX_SKELETON] = sh.skeletal ? 1 : 0;
				const bool ssao = !translucent && any(sh.mesh->flags & MeshRenderFlags::DepthWrite) && any(data.effects.effects & ScreenSpaceEffectsFlags::AmbientOcclusion);
				uniOptions[CAGE_SHADER_OPTIONINDEX_AMBIENTOCCLUSION] = ssao ? 1 : 0;

				std::array<Holder<Texture>, MaxTexturesCountPerMaterial> textures = {};
				for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
				{
					const uint32 n = sh.mesh->textureNames[i];
					textures[i] = assets->get<AssetSchemeIndexTexture, Texture>(n);
					if (textures[i])
					{
						if (i < 3)
						{
							switch (textures[i]->target())
							{
								case GL_TEXTURE_2D_ARRAY:
									renderQueue->bind(textures[i], CAGE_SHADER_TEXTURE_ALBEDO_ARRAY + i);
									break;
								case GL_TEXTURE_CUBE_MAP:
									renderQueue->bind(textures[i], CAGE_SHADER_TEXTURE_ALBEDO_CUBE + i);
									break;
								default:
									renderQueue->bind(textures[i], CAGE_SHADER_TEXTURE_ALBEDO + i);
									break;
							}
						}
						else
							renderQueue->bind(textures[i], CAGE_SHADER_TEXTURE_CUSTOM);
					}
				}
				updateShaderRoutinesForTextures(textures, uniOptions);

				renderQueue->universalUniformStruct(uniOptions, CAGE_SHADER_UNIBLOCK_OPTIONS);

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
				renderQueue->uniform(shaderText, 0, data.viewProj * text.model);
				renderQueue->uniform(shaderText, 4, text.color);
				text.font->render(+renderQueue, modelSquare, text.glyphs, text.format);
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
					renderQueue->bind(shaderText);
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
					if (Holder<Texture> tex = assets->get<AssetSchemeIndexTexture, Texture>(pr.mesh->textureNames[0]))
						pr.uni.aniTexFrames = detail::evalSamplesForTextureAnimation(+tex, currentTime, pt->startTime, pt->speed, pt->offset);
				}

				if (ps)
				{
					if (ps->name && pr.mesh->bones)
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
					if (Holder<SkeletalAnimation> anim = assets->get<AssetSchemeIndexSkeletalAnimation, SkeletalAnimation>(ps->name))
					{
						const Real coefficient = detail::evalCoefficientForSkeletalAnimation(+anim, currentTime, ps->startTime, ps->speed, ps->offset);
						pr.skeletalAnimation = skeletonPreparatorCollection->create(pr.e, std::move(anim), coefficient, pr.mesh->importTransform, cnfRenderSkeletonBones);
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
				uint32 preferredLod = 0;
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
					preferredLod = object->lodSelect(f);
				}

				// try load the preferred lod
				std::vector<Holder<Model>> models;
				bool ok = true;
				const auto &fetch = [&](uint32 lod, bool load)
				{
					models.clear();
					ok = true;
					for (uint32 it : object->models(lod))
					{
						auto md = onDemand->get<AssetSchemeIndexModel, Model>(it, load);
						if (md)
							models.push_back(std::move(md));
						else
							ok = false;
					}
				};
				fetch(preferredLod, true);

				// try acquire one level coarser
				if (!ok && preferredLod + 1 < object->lodsCount())
					fetch(preferredLod + 1, false);

				// try acquire one level finer
				if (!ok && preferredLod > 0)
					fetch(preferredLod - 1, false);

				// render selected lod
				for (auto &it : models)
				{
					ModelPrepare pr = prepare.clone();
					pr.mesh = std::move(it);
					prepareModel<PrepareMode>(data, pr, object.share());
				}
			}

			template<PrepareModeEnum PrepareMode>
			void prepareEntities(CameraData &data) const
			{
				entitiesVisitor(
					[&](Entity *e, const RenderComponent &rc)
					{
						if ((rc.sceneMask & data.camera.sceneMask) == 0)
							return;
						ModelPrepare prepare;
						prepare.e = e;
						prepare.render = rc;
						prepare.model = modelTransform(e);
						prepare.uni = initializeMeshUni(data, prepare.model);
						prepare.frustum = Frustum(prepare.uni.mvpMat);
						if (Holder<RenderObject> obj = assets->get<AssetSchemeIndexRenderObject, RenderObject>(rc.object))
						{
							prepareObject<PrepareMode>(data, prepare, std::move(obj));
							return;
						}
						if (Holder<Model> mesh = assets->get<AssetSchemeIndexModel, Model>(rc.object))
						{
							prepare.mesh = std::move(mesh);
							prepareModel<PrepareMode>(data, prepare);
							return;
						}
						if (cnfRenderMissingModels)
						{
							prepare.mesh = assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/fake.obj"));
							prepareModel<PrepareMode>(data, prepare);
							return;
						}
					},
					+scene, false);

				if constexpr (PrepareMode == PrepareModeEnum::Camera)
				{
					// sort translucent back-to-front
					for (auto &it : data.layers)
						std::sort(it.second.translucent.begin(), it.second.translucent.end(), [](const auto &a, const auto &b) { return a.depth > b.depth; });

					entitiesVisitor(
						[&](Entity *e, const TextComponent &tc_)
						{
							if ((tc_.sceneMask & data.camera.sceneMask) == 0)
								return;
							TextComponent pt = tc_;
							TextPrepare prepare;
							if (!pt.font)
								pt.font = HashString("cage/font/ubuntu/regular.ttf");
							prepare.font = assets->get<AssetSchemeIndexFont, Font>(pt.font);
							if (!prepare.font)
								return;
							const String str = textsGet(pt.textId, pt.value);
							const uint32 count = prepare.font->glyphsCount(str);
							if (count == 0)
								return;
							prepare.glyphs.resize(count);
							prepare.font->transcript(str, prepare.glyphs);
							prepare.color = colorGammaToLinear(pt.color) * pt.intensity;
							prepare.format.size = 1;
							prepare.format.align = pt.align;
							prepare.format.lineSpacing = pt.lineSpacing;
							const Vec2 size = prepare.font->size(prepare.glyphs, prepare.format);
							prepare.format.wrapWidth = size[0];
							prepare.model = modelTransform(e) * Mat4(Vec3(size * Vec2(-0.5, 0.5), 0));
							data.layers[0].texts.push_back(std::move(prepare));
						},
						+scene, false);
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
			}

			void prepareCameraLights(CameraData &data) const
			{
				const Holder<RenderQueue> &renderQueue = data.renderQueue;

				{ // add unshadowed lights
					std::vector<UniLight> lights;
					lights.reserve(CAGE_SHADER_MAX_LIGHTS);
					const Frustum frustum(data.viewProj);
					entitiesVisitor(
						[&](Entity *e, const LightComponent &lc)
						{
							if ((lc.sceneMask & data.camera.sceneMask) == 0)
								return;
							if (e->has<ShadowmapComponent>())
								return;
							UniLight uni = initializeLightUni(modelTransform(e), lc, data.transform.position);
							if (lc.lightType == LightTypeEnum::Point && !intersects(frustum, Sphere(Vec3(uni.position), uni.position[3])))
								return;
							lights.push_back(uni);
						},
						+scene, false);
					filterLightsOverLimit(lights, camera.maxLights);
					data.lightsCount = numeric_cast<uint32>(lights.size());
					if (!lights.empty())
						renderQueue->universalUniformArray<UniLight>(lights, CAGE_SHADER_UNIBLOCK_LIGHTS);
				}

				{ // add shadowed lights
					std::vector<UniShadowedLight> shadows;
					PointerRangeHolder<TextureHandle> tex2d, texCube;
					shadows.reserve(CAGE_SHADER_MAX_SHADOWMAPSCUBE + CAGE_SHADER_MAX_SHADOWMAPS2D);
					tex2d.reserve(CAGE_SHADER_MAX_SHADOWMAPS2D);
					texCube.reserve(CAGE_SHADER_MAX_SHADOWMAPSCUBE);
					for (auto &[e, sh] : data.shadowmaps)
					{
						if (sh.lightComponent.lightType == LightTypeEnum::Point)
						{
							if (texCube.size() == CAGE_SHADER_MAX_SHADOWMAPSCUBE)
								CAGE_THROW_ERROR(Exception, "too many shadowmaps (cube shadows)");
							renderQueue->bind(sh.shadowTexture, CAGE_SHADER_TEXTURE_SHADOWMAPCUBE0 + texCube.size());
							sh.shadowUni.shadowParams[0] = texCube.size();
							sh.shadowUni.shadowParams[1] = shadows.size();
							texCube.push_back(sh.shadowTexture);
						}
						else
						{
							if (tex2d.size() == CAGE_SHADER_MAX_SHADOWMAPS2D)
								CAGE_THROW_ERROR(Exception, "too many shadowmaps (2D shadows)");
							renderQueue->bind(sh.shadowTexture, CAGE_SHADER_TEXTURE_SHADOWMAP2D0 + tex2d.size());
							sh.shadowUni.shadowParams[0] = tex2d.size();
							sh.shadowUni.shadowParams[1] = shadows.size();
							tex2d.push_back(sh.shadowTexture);
						}
						shadows.push_back(sh.shadowUni);
					}
					CAGE_ASSERT(shadows.size() == tex2d.size() + texCube.size());
					data.shadowedLightsCount = numeric_cast<uint32>(shadows.size());
					if (!shadows.empty())
						renderQueue->universalUniformArray<UniShadowedLight>(shadows, CAGE_SHADER_UNIBLOCK_SHADOWEDLIGHTS);
				}
			}

			void taskCamera(CameraData &data, uint32) const
			{
				data.renderQueue = newRenderQueue(data.name + "_camera", provisionalGraphics);
				Holder<RenderQueue> &renderQueue = data.renderQueue;
				const auto graphicsDebugScope = renderQueue->namedScope("camera");

				data.model = Mat4(data.transform);
				data.view = inverse(data.model);
				data.viewProj = data.projection * data.view;
				prepareEntities<PrepareModeEnum::Camera>(data);
				prepareCameraLights(data);

				TextureHandle colorTexture = provisionalGraphics->texture(Stringizer() + "colorTarget_" + data.name + "_" + data.resolution,
					[resolution = data.resolution](Texture *t)
					{
						t->initialize(resolution, 1, GL_RGBA16F); // RGB16F (without alpha) is not required to work in frame buffer
						t->filters(GL_LINEAR, GL_LINEAR, 0);
						t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					});
				TextureHandle depthTexture = provisionalGraphics->texture(Stringizer() + "depthTarget_" + data.name + "_" + data.resolution,
					[resolution = data.resolution](Texture *t)
					{
						t->initialize(resolution, 1, GL_DEPTH_COMPONENT32);
						t->filters(GL_LINEAR, GL_LINEAR, 0);
						t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					});
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

					TextureHandle depthTextureLowRes = [&]()
					{
						const auto graphicsDebugScope = renderQueue->namedScope("lowResDepth");
						TextureHandle t = provisionalGraphics->texture(Stringizer() + "depthTextureLowRes_" + data.name + "_" + data.resolution,
							[ssaoResolution](Texture *t)
							{
								t->initialize(ssaoResolution, 1, GL_R32F);
								t->filters(GL_NEAREST, GL_NEAREST, 0);
								t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
							});

						FrameBufferHandle renderTarget = provisionalGraphics->frameBufferDraw(Stringizer() + "depthTargetLowRes_" + data.name);
						renderQueue->bind(renderTarget);
						renderQueue->colorTexture(renderTarget, 0, t);
						renderQueue->depthTexture(renderTarget, {});
						renderQueue->activeAttachments(renderTarget, 1);
						renderQueue->checkFrameBuffer(renderTarget);
						renderQueue->viewport(Vec2i(), ssaoResolution);
						renderQueue->bind(shaderBlitScaled);
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

				{
					const auto graphicsDebugScope = renderQueue->namedScope("effects");

					TextureHandle texSource = colorTexture;
					TextureHandle texTarget = provisionalGraphics->texture(Stringizer() + "intermediateTarget_" + data.resolution,
						[resolution = data.resolution](Texture *t)
						{
							t->initialize(resolution, 1, GL_RGBA16F); // RGB16F (without alpha) is not required to work in frame buffer
							t->filters(GL_LINEAR, GL_LINEAR, 0);
							t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
						});

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

					renderQueue->resetFrameBuffer();
					renderQueue->resetAllState();
					renderQueue->resetAllTextures();

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
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						cfg.gamma = any(data.effects.effects & ScreenSpaceEffectsFlags::GammaCorrection) ? data.effects.gamma : 1;
						cfg.tonemapEnabled = any(data.effects.effects & ScreenSpaceEffectsFlags::ToneMapping);
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
						const auto graphicsDebugScope = renderQueue->namedScope("effects blit");
						renderQueue->viewport(Vec2i(), data.resolution);
						renderQueue->bind(texSource, 0);
						renderQueue->bind(shaderBlitPixels);
						renderQueue->bind(renderTarget);
						renderQueue->colorTexture(renderTarget, 0, colorTexture);
						renderQueue->depthTexture(renderTarget, {});
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
					renderQueue->bind(data.target, 0); // ensure the texture is properly initialized
					renderQueue->bind(colorTexture, 0);
					renderQueue->bind(shaderBlitPixels);
					renderQueue->bind(renderTarget);
					renderQueue->colorTexture(renderTarget, 0, data.target);
					renderQueue->depthTexture(renderTarget, {});
					renderQueue->activeAttachments(renderTarget, 1);
					renderQueue->checkFrameBuffer(renderTarget);
					renderQueue->draw(modelSquare);
					renderQueue->resetFrameBuffer();
					renderQueue->resetAllState();
					renderQueue->resetAllTextures();
					renderQueue->checkGlErrorDebug();
				}
			}

			Holder<AsyncTask> prepareShadowmap(CameraData &camera, Entity *e, const LightComponent &lc, const ShadowmapComponent &sc) const
			{
				ShadowmapData &data = camera.shadowmaps[e];
				CAGE_ASSERT(e->id() != 0); // lights with shadowmap may not be anonymous
				data.name = Stringizer() + camera.name + "_shadowmap_" + e->id();
				data.camera.sceneMask = camera.camera.sceneMask;
				data.lightComponent = lc;
				data.shadowmapComponent = sc;
				data.resolution = Vec2i(data.shadowmapComponent.resolution);
				data.model = modelTransform(e);
				data.view = inverse(data.model);
				data.projection = [&]()
				{
					const auto &sc = data.shadowmapComponent;
					switch (data.lightComponent.lightType)
					{
						case LightTypeEnum::Directional:
							return orthographicProjection(-sc.directionalWorldSize, sc.directionalWorldSize, -sc.directionalWorldSize, sc.directionalWorldSize, -sc.directionalWorldSize, sc.directionalWorldSize);
						case LightTypeEnum::Spot:
							return perspectiveProjection(data.lightComponent.spotAngle, 1, lc.minDistance, lc.maxDistance);
						case LightTypeEnum::Point:
							return perspectiveProjection(Degs(90), 1, lc.minDistance, lc.maxDistance);
						default:
							CAGE_THROW_CRITICAL(Exception, "invalid light type");
					}
				}();
				data.viewProj = data.projection * data.view;
				data.lodSelection = camera.lodSelection;
				static constexpr Mat4 bias = Mat4(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.5, 0.5, 1.0);
				data.shadowUni.shadowMat = bias * data.viewProj;

				{
					const String name = Stringizer() + data.name + "_" + data.resolution;
					data.shadowTexture = provisionalGraphics->texture(name, data.lightComponent.lightType == LightTypeEnum::Point ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D,
						[resolution = data.resolution, format = data.lightComponent.lightType == LightTypeEnum::Point ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT24](Texture *t)
						{
							t->initialize(resolution, 1, format);
							t->filters(GL_LINEAR, GL_LINEAR, 16);
							t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
						});
				}

				{
					UniShadowedLight &uni = data.shadowUni;
					(UniLight &)uni = initializeLightUni(data.model, data.lightComponent, data.transform.position);
					uni.shadowParams[2] = data.shadowmapComponent.normalOffsetScale;
					uni.shadowParams[3] = data.shadowmapComponent.shadowFactor;
					uni.params[0] += 1; // shadowed light type
				}

				return tasksRunAsync<ShadowmapData>("render shadowmap task", Delegate<void(ShadowmapData &, uint32)>().bind<RenderPipelineImpl, &RenderPipelineImpl::taskShadowmap>(this), Holder<ShadowmapData>(&data, nullptr));
			}

			Holder<RenderQueue> prepareCamera()
			{
				CAGE_ASSERT(!name.empty());

				CameraData data;
				data.name = name;
				data.effects = effects;
				data.projection = projection;
				data.transform = transform;
				data.target = target;
				data.camera = camera;
				data.lodSelection = lodSelection;
				data.resolution = resolution;

				std::vector<Holder<AsyncTask>> tasks;
				entitiesVisitor(
					[&](Entity *e, const LightComponent &lc, const ShadowmapComponent &sc)
					{
						if ((lc.sceneMask & data.camera.sceneMask) == 0)
							return;
						tasks.push_back(prepareShadowmap(data, e, lc, sc));
					},
					+scene, false);
				tasks.push_back(tasksRunAsync<CameraData>("render camera task", Delegate<void(CameraData &, uint32)>().bind<RenderPipelineImpl, &RenderPipelineImpl::taskCamera>(this), Holder<CameraData>(&data, nullptr)));
				for (const auto &it : tasks)
					it->wait();

				Holder<RenderQueue> queue = newRenderQueue(name + "_pipeline", provisionalGraphics);

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
					viewport.viewport = Vec4(Vec2(), Vec2(data.resolution));
					viewport.time = Vec4(frameIndex % 10000, (currentTime % uint64(1e6)) / 1e6, (currentTime % uint64(1e9)) / 1e9, 0);
					queue->universalUniformStruct(viewport, CAGE_SHADER_UNIBLOCK_VIEWPORT);
				}

				// ensure that shadowmaps are rendered before the camera
				for (auto &shm : data.shadowmaps)
					queue->enqueue(std::move(shm.second.renderQueue));

				queue->enqueue(std::move(data.renderQueue));
				return queue;
			}
		};
	}

	Holder<RenderQueue> renderPipeline(const RenderPipelineConfig &config)
	{
		CAGE_ASSERT(config.assets);
		CAGE_ASSERT(config.provisionalGraphics);
		CAGE_ASSERT(config.scene);
		CAGE_ASSERT(config.onDemand);
		CAGE_ASSERT(config.target);
		CAGE_ASSERT(config.resolution[0] > 0);
		CAGE_ASSERT(config.resolution[1] > 0);
		CAGE_ASSERT(valid(config.projection));
		CAGE_ASSERT(valid(config.transform.orientation));
		CAGE_ASSERT(valid(config.transform.position));
		CAGE_ASSERT(valid(config.transform.scale));
		CAGE_ASSERT(valid(config.lodSelection.center));
		CAGE_ASSERT(valid(config.lodSelection.screenSize));
		CAGE_ASSERT(valid(config.interpolationFactor));

		RenderPipelineImpl impl(config);
		if (impl.skeletonPreparatorCollection)
			return impl.prepareCamera();
		return {};
	}
}
