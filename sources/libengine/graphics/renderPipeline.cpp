#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <variant>
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
	namespace detail
	{
		extern uint64 GuiTextFontDefault;
	}

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

			sint32 &operator[](uint32 index)
			{
				CAGE_ASSERT(index < CAGE_SHADER_MAX_OPTIONS);
				return options[index / 4][index % 4];
			}
		};

		struct RenderModel : private Noncopyable
		{
			std::optional<TextureAnimationComponent> textureAnimation;
			Vec4 aniTexFrames = Vec4::Nan();
			Holder<SkeletalAnimationPreparatorInstance> skeletalAnimation;
			Holder<Model> mesh;
		};

		struct RenderText : private Noncopyable
		{
			FontLayoutResult layout;
			Holder<Font> font;
		};

		struct RenderData : private Noncopyable
		{
			std::variant<std::monostate, RenderModel, RenderText> data;
			Mat4 model;
			Vec4 color = Vec4::Nan();
			Entity *e = nullptr;
			sint32 renderLayer = 0;
			bool translucent = false; // transparent or fade
			Real depth; // for sorting transparent objects only

			RenderData() = default;

			RenderData(RenderData &&other) noexcept
			{
				// performance hack
				detail::memcpy(this, &other, sizeof(RenderData));
				detail::memset(&other, 0, sizeof(RenderData));
			}

			RenderData &operator=(RenderData &&other) noexcept
			{
				// performance hack
				if (&other == this)
					return *this;
				this->~RenderData();
				detail::memcpy(this, &other, sizeof(RenderData));
				detail::memset(&other, 0, sizeof(RenderData));
				return *this;
			}

			auto cmp() const noexcept
			{
				return std::visit(
					[this](const auto &r) -> std::tuple<sint32, bool, Real, void *, bool>
					{
						using T = std::decay_t<decltype(r)>;
						if constexpr (std::is_same_v<T, RenderModel>)
							return { renderLayer, translucent, depth, +r.mesh, !!r.skeletalAnimation };
						if constexpr (std::is_same_v<T, RenderText>)
							return { renderLayer, translucent, depth, +r.font, false };
						return {};
					},
					data);
			}
		};

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

		struct CameraData : private Noncopyable
		{
			std::map<Entity *, Holder<struct RenderPipelineImpl>> shadowmaps;
			uint32 lightsCount = 0;
			uint32 shadowedLightsCount = 0;
		};

		struct ShadowmapData : private Noncopyable
		{
			UniShadowedLight shadowUni;
			Holder<ProvisionalTexture> shadowTexture;
			LightComponent lightComponent;
			ShadowmapComponent shadowmapComponent;
		};

		struct RenderPipelineImpl : public RenderPipelineConfig
		{
			Holder<Model> modelSquare, modelBone;
			Holder<ShaderProgram> shaderBlitPixels, shaderBlitScaled;
			Holder<ShaderProgram> shaderDepth, shaderStandard, shaderDepthCutOut, shaderStandardCutOut;
			Holder<ShaderProgram> shaderText;

			Holder<SkeletalAnimationPreparatorCollection> skeletonPreparatorCollection;
			EntityComponent *transformComponent = nullptr;
			EntityComponent *prevTransformComponent = nullptr;

			Mat4 model;
			Mat4 view;
			Mat4 viewProj;
			Frustum frustum;
			std::optional<CameraData> data;
			std::optional<ShadowmapData> shadowmap;
			std::vector<RenderData> renderData;
			Holder<RenderQueue> renderQueue;
			mutable std::vector<UniMesh> uniMeshes;
			mutable std::vector<Mat3x4> uniArmatures;
			mutable std::vector<float> uniCustomData;
			mutable std::vector<Holder<Model>> prepareModels;
			bool cnfRenderMissingModels = confRenderMissingModels;
			bool cnfRenderSkeletonBones = confRenderSkeletonBones;

			// create pipeline for regular camera
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

			// create pipeline for shadowmap
			explicit RenderPipelineImpl(const RenderPipelineImpl *camera) : RenderPipelineConfig(*camera)
			{
				CAGE_ASSERT(camera->skeletonPreparatorCollection);

#define SHARE(NAME) NAME = camera->NAME.share();
				SHARE(modelSquare);
				SHARE(modelBone);
				SHARE(shaderBlitPixels);
				SHARE(shaderBlitScaled);
				SHARE(shaderDepth);
				SHARE(shaderStandard);
				SHARE(shaderDepthCutOut);
				SHARE(shaderStandardCutOut);
				SHARE(shaderText);
				SHARE(skeletonPreparatorCollection);
#undef SHARE

				transformComponent = camera->transformComponent;
				prevTransformComponent = camera->prevTransformComponent;
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

			UniMesh makeMeshUni(const RenderData &rd) const
			{
				CAGE_ASSERT(std::holds_alternative<RenderModel>(rd.data));
				const RenderModel &rm = std::get<RenderModel>(rd.data);
				UniMesh uni;
				uni.mvpMat = viewProj * rd.model;
				uni.normalMat = Mat3x4(inverse(Mat3(rd.model)));
				uni.normalMat.data[2][3] = any(rm.mesh->flags & MeshRenderFlags::Lighting) ? 1 : 0; // is lighting enabled
				uni.normalMat.data[1][3] = any(rm.mesh->flags & MeshRenderFlags::Fade) ? 1 : 0; // transparent mode is to fade
				uni.mMat = Mat3x4(rd.model);
				uni.color = rd.color;
				uni.aniTexFrames = rm.aniTexFrames;
				return uni;
			}

			static UniLight initializeLightUni(const Mat4 &model, const LightComponent &lc, const Vec3 &cameraCenter)
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

			static void filterLightsOverLimit(std::vector<UniLight> &lights, uint32 limit)
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

			static void updateShaderRoutinesForTextures(const std::array<Holder<Texture>, MaxTexturesCountPerMaterial> &textures, UniOptions &options)
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

			void renderModels(const RenderModeEnum renderMode, const PointerRange<const RenderData> instances) const
			{
				CAGE_ASSERT(!instances.empty());
				const RenderData &rd = instances[0];
				CAGE_ASSERT(std::holds_alternative<RenderModel>(rd.data));
				const RenderModel &rm = std::get<RenderModel>(rd.data);

				if (renderMode != RenderModeEnum::Color && rd.translucent)
					return;
				if (renderMode == RenderModeEnum::DepthPrepass && none(rm.mesh->flags & MeshRenderFlags::DepthWrite))
					return;

				Holder<ShaderProgram> shader = [&]()
				{
					if (rm.mesh->shaderName)
					{
						uint32 variant = 0;
						if (renderMode != RenderModeEnum::Color)
							variant += HashString("DepthOnly");
						if (any(rm.mesh->flags & MeshRenderFlags::CutOut))
							variant += HashString("CutOut");
						return assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(rm.mesh->shaderName)->get(variant);
					}
					else
					{
						if (renderMode == RenderModeEnum::Color)
						{ // color
							if (any(rm.mesh->flags & MeshRenderFlags::CutOut))
								return shaderStandardCutOut.share();
							else
								return shaderStandard.share();
						}
						else
						{ // depth
							if (any(rm.mesh->flags & MeshRenderFlags::CutOut))
								return shaderDepthCutOut.share();
							else
								return shaderDepth.share();
						}
					}
				}();
				renderQueue->bind(shader);

				renderQueue->culling(none(rm.mesh->flags & MeshRenderFlags::TwoSided));
				renderQueue->depthTest(any(rm.mesh->flags & MeshRenderFlags::DepthTest));
				if (renderMode == RenderModeEnum::Color)
				{
					renderQueue->depthWrite(any(rm.mesh->flags & MeshRenderFlags::DepthWrite));
					renderQueue->depthFuncLessEqual();
					renderQueue->blending(rd.translucent);
					if (rd.translucent)
						renderQueue->blendFuncPremultipliedTransparency();
				}
				else
				{
					renderQueue->depthWrite(true);
					renderQueue->depthFuncLess();
					renderQueue->blending(false);
				}

				UniOptions uniOptions;
				uniOptions[CAGE_SHADER_OPTIONINDEX_BONESCOUNT] = rm.mesh->bones;
				uniOptions[CAGE_SHADER_OPTIONINDEX_LIGHTSCOUNT] = data->lightsCount;
				uniOptions[CAGE_SHADER_OPTIONINDEX_SHADOWEDLIGHTSCOUNT] = data->shadowedLightsCount;
				uniOptions[CAGE_SHADER_OPTIONINDEX_SKELETON] = rm.skeletalAnimation ? 1 : 0;
				const bool ssao = !rd.translucent && any(rm.mesh->flags & MeshRenderFlags::DepthWrite) && any(effects.effects & ScreenSpaceEffectsFlags::AmbientOcclusion);
				uniOptions[CAGE_SHADER_OPTIONINDEX_AMBIENTOCCLUSION] = ssao ? 1 : 0;

				std::array<Holder<Texture>, MaxTexturesCountPerMaterial> textures = {};
				for (uint32 i = 0; i < MaxTexturesCountPerMaterial; i++)
				{
					const uint32 n = rm.mesh->textureNames[i];
					if (n == 0)
						continue;
					textures[i] = assets->get<AssetSchemeIndexTexture, Texture>(n);
					if (!textures[i])
						continue;
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
				updateShaderRoutinesForTextures(textures, uniOptions);

				renderQueue->universalUniformStruct(uniOptions, CAGE_SHADER_UNIBLOCK_OPTIONS);

				// separate instances into draw calls by the limit
				const uint32 limit = rm.skeletalAnimation ? min(uint32(CAGE_SHADER_MAX_MESHES), CAGE_SHADER_MAX_BONES / rm.mesh->bones) : CAGE_SHADER_MAX_MESHES;
				for (uint32 offset = 0; offset < instances.size(); offset += limit)
				{
					const uint32 count = min(limit, numeric_cast<uint32>(instances.size()) - offset);
					uniMeshes.clear();
					uniMeshes.reserve(count);
					uniArmatures.clear();
					if (rm.skeletalAnimation)
						uniArmatures.reserve(rm.mesh->bones * count);
					CAGE_ASSERT(shader->customDataCount <= 16);
					uniCustomData.clear();
					if (shader->customDataCount)
						uniCustomData.reserve(shader->customDataCount * count);

					for (const auto &inst : subRange(instances, offset, count))
					{
						CAGE_ASSERT(std::holds_alternative<RenderModel>(inst.data));
						const RenderModel &r = std::get<RenderModel>(inst.data);
						CAGE_ASSERT(+r.mesh == +rm.mesh);
						uniMeshes.push_back(makeMeshUni(inst));
						if (rm.skeletalAnimation)
						{
							const auto a = r.skeletalAnimation->armature();
							CAGE_ASSERT(a.size() == rm.mesh->bones);
							for (const auto &b : a)
								uniArmatures.push_back(b);
						}
						if (shader->customDataCount)
						{
							if (inst.e && inst.e->has<ShaderDataComponent>())
							{
								const auto sub = subRange(PointerRange<const Real>(inst.e->value<ShaderDataComponent>().matrix.data).cast<const float>(), 0, shader->customDataCount);
								for (const auto it : sub)
									uniCustomData.push_back(it);
							}
							else
							{
								for (uint32 i = 0; i < shader->customDataCount; i++)
									uniCustomData.push_back({});
							}
						}
					}

					renderQueue->universalUniformArray<const UniMesh>(uniMeshes, CAGE_SHADER_UNIBLOCK_MESHES);
					if (!uniArmatures.empty())
						renderQueue->universalUniformArray<const Mat3x4>(uniArmatures, CAGE_SHADER_UNIBLOCK_ARMATURES);
					if (!uniCustomData.empty())
						renderQueue->universalUniformArray<const float>(uniCustomData, CAGE_SHADER_UNIBLOCK_CUSTOMDATA);
					renderQueue->draw(rm.mesh, count);
				}

				renderQueue->checkGlErrorDebug();
			}

			void renderTexts(const RenderModeEnum renderMode, const PointerRange<const RenderData> instances) const
			{
				CAGE_ASSERT(!instances.empty());
				const RenderData &rd = instances[0];
				CAGE_ASSERT(std::holds_alternative<RenderText>(rd.data));

				renderQueue->depthTest(true);
				renderQueue->depthWrite(false);
				renderQueue->culling(true);
				renderQueue->blending(true);
				renderQueue->blendFuncAlphaTransparency();
				renderQueue->bind(shaderText);

				for (const auto &it : instances)
				{
					CAGE_ASSERT(std::holds_alternative<RenderText>(it.data));
					const RenderText &t = std::get<RenderText>(it.data);
					renderQueue->uniform(shaderText, 0, viewProj * it.model);
					renderQueue->uniform(shaderText, 4, Vec3(it.color)); // todo opacity
					t.font->render(+renderQueue, onDemand, t.layout);
				}

				renderQueue->checkGlErrorDebug();
			}

			void renderInstances(const RenderModeEnum renderMode, const PointerRange<const RenderData> instances) const
			{
				CAGE_ASSERT(!instances.empty());
				const RenderData &rd = instances[0];
				if (renderMode == RenderModeEnum::DepthPrepass && rd.translucent)
					return;
				std::visit(
					[&](const auto &r)
					{
						using T = std::decay_t<decltype(r)>;
						if constexpr (std::is_same_v<T, RenderModel>)
							renderModels(renderMode, instances);
						if constexpr (std::is_same_v<T, RenderText>)
							renderTexts(renderMode, instances);
					},
					rd.data);
			}

			void renderPass(const RenderModeEnum renderMode) const
			{
				switch (renderMode)
				{
					case RenderModeEnum::Shadowmap:
						CAGE_ASSERT(this->shadowmap);
						CAGE_ASSERT(!this->data);
						break;
					default:
						CAGE_ASSERT(!this->shadowmap);
						CAGE_ASSERT(this->data);
						break;
				}

				const auto &mark = [](const RenderData &d)
				{
					return std::visit(
						[&](const auto &r) -> std::tuple<void *, bool, bool>
						{
							using T = std::decay_t<decltype(r)>;
							if constexpr (std::is_same_v<T, RenderModel>)
								return { +r.mesh, !!r.skeletalAnimation, d.translucent };
							if constexpr (std::is_same_v<T, RenderText>)
								return { +r.font, false, d.translucent };
							return {};
						},
						d.data);
				};
				auto it = renderData.begin();
				const auto et = renderData.end();
				while (it != et)
				{
					const auto mk = mark(*it);
					auto i = it + 1;
					while (i != et && mark(*i) == mk)
						i++;
					PointerRange<const RenderData> instances = { &*it, &*i };
					renderInstances(renderMode, instances);
					it = i;
				}

				renderQueue->resetAllState();
				renderQueue->checkGlErrorDebug();
			}

			void prepareModelBones(const RenderData &rd)
			{
				CAGE_ASSERT(std::holds_alternative<RenderModel>(rd.data));
				const RenderModel &rm = std::get<RenderModel>(rd.data);
				CAGE_ASSERT(rm.mesh);
				CAGE_ASSERT(rm.skeletalAnimation);
				const auto armature = rm.skeletalAnimation->armature();
				for (uint32 i = 0; i < armature.size(); i++)
				{
					RenderData d;
					d.model = rd.model * Mat4(armature[i]);
					d.color = Vec4(colorGammaToLinear(colorHsvToRgb(Vec3(Real(i) / Real(armature.size()), 1, 1))), 1);
					d.e = rd.e;
					d.renderLayer = rd.renderLayer;
					RenderModel r;
					r.mesh = modelBone.share();
					d.data = std::move(r);
					renderData.push_back(std::move(d));
				}
			}

			void prepareModel(RenderData &rd, const RenderObject *parent = {})
			{
				CAGE_ASSERT(std::holds_alternative<RenderModel>(rd.data));
				RenderModel &rm = std::get<RenderModel>(rd.data);
				if (!rm.mesh)
					return;

				if (shadowmap && none(rm.mesh->flags & MeshRenderFlags::ShadowCast))
					return;

				const Mat4 mvpMat = viewProj * rd.model;
				if (!intersects(rm.mesh->boundingBox(), Frustum(mvpMat)))
					return;

				std::optional<TextureAnimationComponent> &pt = rm.textureAnimation;
				if (rd.e->has<TextureAnimationComponent>())
					pt = rd.e->value<TextureAnimationComponent>();

				std::optional<SkeletalAnimationComponent> ps;
				if (rd.e->has<SkeletalAnimationComponent>())
					ps = rd.e->value<SkeletalAnimationComponent>();

				RenderComponent render = rd.e->value<RenderComponent>();
				if (parent)
				{
					if (!render.color.valid())
						render.color = parent->color;
					if (!render.intensity.valid())
						render.intensity = parent->intensity;
					if (!render.opacity.valid())
						render.opacity = parent->opacity;

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

				if (!render.color.valid())
					render.color = Vec3(0);
				if (!render.intensity.valid())
					render.intensity = 1;
				if (!render.opacity.valid())
					render.opacity = 1;
				rd.color = Vec4(colorGammaToLinear(render.color) * render.intensity, render.opacity);

				if (pt)
				{
					if (!pt->speed.valid())
						pt->speed = 1;
					if (!pt->offset.valid())
						pt->offset = 0;
					if (rm.mesh->textureNames[0])
						if (Holder<Texture> tex = assets->get<AssetSchemeIndexTexture, Texture>(rm.mesh->textureNames[0]))
							rm.aniTexFrames = detail::evalSamplesForTextureAnimation(+tex, currentTime, pt->startTime, pt->speed, pt->offset);
				}

				if (ps)
				{
					if (ps->name && rm.mesh->bones)
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
						rm.skeletalAnimation = skeletonPreparatorCollection->create(rd.e, std::move(anim), coefficient, rm.mesh->importTransform, cnfRenderSkeletonBones);
						rm.skeletalAnimation->prepare();
					}
					else
						ps.reset();
				}

				rd.renderLayer = render.layer + rm.mesh->layer;
				rd.translucent = any(rm.mesh->flags & (MeshRenderFlags::Transparent | MeshRenderFlags::Fade)) || render.opacity < 1;
				if (rd.translucent)
					rd.depth = (mvpMat * Vec4(0, 0, 0, 1))[2] * -1;

				if (rm.skeletalAnimation && cnfRenderSkeletonBones)
					prepareModelBones(rd);
				else
					renderData.push_back(std::move(rd));
			}

			void prepareObject(const RenderData &rd, Holder<RenderObject> object)
			{
				CAGE_ASSERT(object->lodsCount() > 0);
				CAGE_ASSERT(std::holds_alternative<std::monostate>(rd.data));

				uint32 preferredLod = 0;
				if (object->lodsCount() > 1)
				{
					Real d = 1;
					if (!lodSelection.orthographic)
					{
						const Vec4 ep4 = rd.model * Vec4(0, 0, 0, 1);
						CAGE_ASSERT(abs(ep4[3] - 1) < 1e-4);
						d = distance(Vec3(ep4), lodSelection.center);
					}
					const Real f = lodSelection.screenSize * object->worldSize / (d * object->pixelsSize);
					preferredLod = object->lodSelect(f);
				}

				// try load the preferred lod
				bool ok = true;
				const auto &fetch = [&](uint32 lod, bool load)
				{
					prepareModels.clear();
					ok = true;
					for (uint32 it : object->models(lod))
					{
						if (auto md = onDemand->get<AssetSchemeIndexModel, Model>(it, load))
							prepareModels.push_back(std::move(md));
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
				for (auto &it : prepareModels)
				{
					RenderData d;
					d.model = rd.model;
					d.e = rd.e;
					RenderModel r;
					r.mesh = std::move(it);
					d.data = std::move(r);
					prepareModel(d, +object);
				}
			}

			void prepareText(Entity *e, TextComponent tc)
			{
				if (!tc.font)
					tc.font = detail::GuiTextFontDefault;
				if (!tc.font)
					tc.font = HashString("cage/font/ubuntu/regular.ttf");
				RenderText rt;
				rt.font = assets->get<AssetSchemeIndexFont, Font>(tc.font);
				if (!rt.font)
					return;
				FontFormat format;
				format.size = 1;
				format.align = tc.align;
				format.lineSpacing = tc.lineSpacing;
				const String str = textsGet(tc.textId, tc.value);
				rt.layout = rt.font->layout(str, format);
				if (rt.layout.glyphs.empty())
					return;
				RenderData rd;
				rd.model = modelTransform(e) * Mat4(Vec3(rt.layout.size * Vec2(-0.5, 0.5), 0));
				rd.color = Vec4(colorGammaToLinear(tc.color) * tc.intensity, 1); // todo opacity
				rd.renderLayer = tc.renderLayer;
				rd.translucent = true;
				rd.depth = (viewProj * (rd.model * Vec4(0, 0, 0, 1)))[2] * -1;
				rd.data = std::move(rt);
				renderData.push_back(std::move(rd));
			}

			void prepareEntities()
			{
				ProfilingScope profiling("prepare entities");
				profiling.set(Stringizer() + "entities: " + scene->count());

				renderData.reserve(scene->component<RenderComponent>()->count() + scene->component<TextComponent>()->count());

				entitiesVisitor(
					[&](Entity *e, const RenderComponent &rc)
					{
						if ((rc.sceneMask & camera.sceneMask) == 0)
							return;
						RenderData rd;
						rd.model = modelTransform(e);
						rd.e = e;
						if (Holder<RenderObject> obj = assets->get<AssetSchemeIndexRenderObject, RenderObject>(rc.object))
						{
							prepareObject(rd, std::move(obj));
							return;
						}
						if (Holder<Model> mesh = assets->get<AssetSchemeIndexModel, Model>(rc.object))
						{
							RenderModel rm;
							rm.mesh = std::move(mesh);
							rd.data = std::move(rm);
							prepareModel(rd);
							return;
						}
						if (cnfRenderMissingModels)
						{
							RenderModel rm;
							rm.mesh = assets->get<AssetSchemeIndexModel, Model>(HashString("cage/model/fake.obj"));
							rd.data = std::move(rm);
							prepareModel(rd);
							return;
						}
					},
					+scene, false);

				entitiesVisitor(
					[&](Entity *e, const TextComponent &tc)
					{
						if ((tc.sceneMask & camera.sceneMask) == 0)
							return;
						prepareText(e, tc);
					},
					+scene, false);
			}

			void orderRenderData()
			{
				ProfilingScope profiling("order render data");
				profiling.set(Stringizer() + "count: " + renderData.size());
				std::sort(renderData.begin(), renderData.end(), [](const RenderData &a, const RenderData &b) { return a.cmp() < b.cmp(); });
			}

			void taskShadowmap()
			{
				CAGE_ASSERT(!data);
				CAGE_ASSERT(shadowmap);

				renderQueue = newRenderQueue(name, provisionalGraphics);
				const auto graphicsDebugScope = renderQueue->namedScope("shadowmap");

				FrameBufferHandle renderTarget = provisionalGraphics->frameBufferDraw("renderTarget");
				renderQueue->bind(renderTarget);
				renderQueue->clearFrameBuffer(renderTarget);
				renderQueue->depthTexture(renderTarget, shadowmap->shadowTexture);
				renderQueue->activeAttachments(renderTarget, 0);
				renderQueue->checkFrameBuffer(renderTarget);
				renderQueue->viewport(Vec2i(), resolution);
				renderQueue->colorWrite(false);
				renderQueue->clear(false, true);
				renderQueue->bind(shaderDepth);
				renderQueue->checkGlErrorDebug();

				prepareEntities();
				orderRenderData();

				{
					const auto graphicsDebugScope = renderQueue->namedScope("shadowmap pass");
					renderPass(RenderModeEnum::Shadowmap);
				}

				renderQueue->resetFrameBuffer();
				renderQueue->resetAllTextures();
				renderQueue->checkGlErrorDebug();
			}

			void prepareCameraLights()
			{
				ProfilingScope profiling("prepare lights");

				CAGE_ASSERT(data);

				{ // add unshadowed lights
					std::vector<UniLight> lights;
					lights.reserve(CAGE_SHADER_MAX_LIGHTS);
					entitiesVisitor(
						[&](Entity *e, const LightComponent &lc)
						{
							if ((lc.sceneMask & camera.sceneMask) == 0)
								return;
							if (e->has<ShadowmapComponent>())
								return;
							UniLight uni = initializeLightUni(modelTransform(e), lc, transform.position);
							if (lc.lightType == LightTypeEnum::Point && !intersects(Sphere(Vec3(uni.position), lc.maxDistance), frustum))
								return;
							lights.push_back(uni);
						},
						+scene, false);
					filterLightsOverLimit(lights, camera.maxLights);
					data->lightsCount = numeric_cast<uint32>(lights.size());
					if (!lights.empty())
						renderQueue->universalUniformArray<UniLight>(lights, CAGE_SHADER_UNIBLOCK_LIGHTS);
				}

				{ // add shadowed lights
					std::vector<UniShadowedLight> shadows;
					PointerRangeHolder<TextureHandle> tex2d, texCube;
					shadows.reserve(CAGE_SHADER_MAX_SHADOWMAPSCUBE + CAGE_SHADER_MAX_SHADOWMAPS2D);
					tex2d.reserve(CAGE_SHADER_MAX_SHADOWMAPS2D);
					texCube.reserve(CAGE_SHADER_MAX_SHADOWMAPSCUBE);
					for (auto &[e, sh_] : data->shadowmaps)
					{
						CAGE_ASSERT(sh_->shadowmap);
						ShadowmapData &sh = *sh_->shadowmap;
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
					data->shadowedLightsCount = numeric_cast<uint32>(shadows.size());
					if (!shadows.empty())
						renderQueue->universalUniformArray<UniShadowedLight>(shadows, CAGE_SHADER_UNIBLOCK_SHADOWEDLIGHTS);
				}

				profiling.set(Stringizer() + "count: " + data->lightsCount + ", shadowed: " + data->shadowedLightsCount);
			}

			void taskCamera(uint32)
			{
				CAGE_ASSERT(data);
				CAGE_ASSERT(!shadowmap);

				renderQueue = newRenderQueue(name + "_camera", provisionalGraphics);
				const auto graphicsDebugScope = renderQueue->namedScope("camera");

				model = Mat4(transform);
				view = inverse(model);
				viewProj = projection * view;
				frustum = Frustum(viewProj);
				prepareEntities();
				orderRenderData();
				prepareCameraLights();

				TextureHandle depthTexture = provisionalGraphics->texture(Stringizer() + "depthTarget_" + name + "_" + resolution,
					[resolution = resolution](Texture *t)
					{
						t->initialize(resolution, 1, GL_DEPTH_COMPONENT32);
						t->filters(GL_LINEAR, GL_LINEAR, 0);
						t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					});

				{
					const auto graphicsDebugScope = renderQueue->namedScope("depth prepass");

					FrameBufferHandle renderTarget = provisionalGraphics->frameBufferDraw("renderTargetDepth");
					renderQueue->bind(renderTarget);
					renderQueue->depthTexture(renderTarget, depthTexture);
					renderQueue->checkFrameBuffer(renderTarget);
					renderQueue->checkGlErrorDebug();
					renderQueue->viewport(Vec2i(), resolution);
					renderQueue->clear(false, true);

					renderPass(RenderModeEnum::DepthPrepass);
				}

				ScreenSpaceCommonConfig commonConfig; // helper to simplify initialization
				commonConfig.assets = assets;
				commonConfig.provisionals = +provisionalGraphics;
				commonConfig.queue = +renderQueue;
				commonConfig.resolution = resolution;

				if (any(effects.effects & ScreenSpaceEffectsFlags::AmbientOcclusion))
				{
					const Vec2i ssaoResolution = resolution / 3;

					TextureHandle depthTextureLowRes = [&]()
					{
						const auto graphicsDebugScope = renderQueue->namedScope("lowResDepth");
						TextureHandle t = provisionalGraphics->texture(Stringizer() + "depthTextureLowRes_" + name + "_" + resolution,
							[ssaoResolution](Texture *t)
							{
								t->initialize(ssaoResolution, 1, GL_R32F);
								t->filters(GL_NEAREST, GL_NEAREST, 0);
								t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
							});

						FrameBufferHandle renderTarget = provisionalGraphics->frameBufferDraw(Stringizer() + "depthTargetLowRes_" + name);
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
					(ScreenSpaceAmbientOcclusion &)cfg = effects.ssao;
					cfg.resolution = ssaoResolution;
					cfg.proj = projection;
					cfg.inDepth = depthTextureLowRes;
					cfg.frameIndex = frameIndex;
					screenSpaceAmbientOcclusion(cfg);

					// bind the texture for sampling
					renderQueue->bind(cfg.outAo, CAGE_SHADER_TEXTURE_SSAO);
				}

				TextureHandle colorTexture = provisionalGraphics->texture(Stringizer() + "colorTarget_" + name + "_" + resolution,
					[resolution = resolution](Texture *t)
					{
						t->initialize(resolution, 1, GL_RGBA16F); // RGB16F (without alpha) is not required to work in frame buffer
						t->filters(GL_LINEAR, GL_LINEAR, 0);
						t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
					});
				FrameBufferHandle renderTarget = provisionalGraphics->frameBufferDraw("renderTargetColor");
				renderQueue->bind(renderTarget);
				renderQueue->depthTexture(renderTarget, depthTexture);
				renderQueue->colorTexture(renderTarget, 0, colorTexture);
				renderQueue->activeAttachments(renderTarget, 1);
				renderQueue->checkFrameBuffer(renderTarget);
				renderQueue->checkGlErrorDebug();
				renderQueue->bind(depthTexture, CAGE_SHADER_TEXTURE_DEPTH);
				renderQueue->viewport(Vec2i(), resolution);
				renderQueue->clear(true, false);

				{
					const auto graphicsDebugScope = renderQueue->namedScope("standard pass");
					renderPass(RenderModeEnum::Color);
				}

				{
					const auto graphicsDebugScope = renderQueue->namedScope("effects");

					TextureHandle texSource = colorTexture;
					TextureHandle texTarget = provisionalGraphics->texture(Stringizer() + "intermediateTarget_" + resolution,
						[resolution = resolution](Texture *t)
						{
							t->initialize(resolution, 1, GL_RGBA16F); // RGB16F (without alpha) is not required to work in frame buffer
							t->filters(GL_LINEAR, GL_LINEAR, 0);
							t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
						});

					// depth of field
					if (any(effects.effects & ScreenSpaceEffectsFlags::DepthOfField))
					{
						ScreenSpaceDepthOfFieldConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						(ScreenSpaceDepthOfField &)cfg = effects.depthOfField;
						cfg.proj = projection;
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
					if (any(effects.effects & ScreenSpaceEffectsFlags::EyeAdaptation))
					{
						ScreenSpaceEyeAdaptationConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						(ScreenSpaceEyeAdaptation &)cfg = effects.eyeAdaptation;
						cfg.cameraId = name;
						cfg.inColor = texSource;
						cfg.elapsedTime = elapsedTime * 1e-6;
						screenSpaceEyeAdaptationPrepare(cfg);
					}

					// bloom
					if (any(effects.effects & ScreenSpaceEffectsFlags::Bloom))
					{
						ScreenSpaceBloomConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						(ScreenSpaceBloom &)cfg = effects.bloom;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						screenSpaceBloom(cfg);
						std::swap(texSource, texTarget);
					}

					// eye adaptation
					if (any(effects.effects & ScreenSpaceEffectsFlags::EyeAdaptation))
					{
						ScreenSpaceEyeAdaptationConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						(ScreenSpaceEyeAdaptation &)cfg = effects.eyeAdaptation;
						cfg.cameraId = name;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						cfg.elapsedTime = elapsedTime * 1e-6;
						screenSpaceEyeAdaptationApply(cfg);
						std::swap(texSource, texTarget);
					}

					// tonemap & gamma correction
					if (any(effects.effects & (ScreenSpaceEffectsFlags::ToneMapping | ScreenSpaceEffectsFlags::GammaCorrection)))
					{
						ScreenSpaceTonemapConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						cfg.gamma = any(effects.effects & ScreenSpaceEffectsFlags::GammaCorrection) ? effects.gamma : 1;
						cfg.tonemapEnabled = any(effects.effects & ScreenSpaceEffectsFlags::ToneMapping);
						screenSpaceTonemap(cfg);
						std::swap(texSource, texTarget);
					}

					// fxaa
					if (any(effects.effects & ScreenSpaceEffectsFlags::AntiAliasing))
					{
						ScreenSpaceFastApproximateAntiAliasingConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						screenSpaceFastApproximateAntiAliasing(cfg);
						std::swap(texSource, texTarget);
					}

					// sharpening
					if (any(effects.effects & ScreenSpaceEffectsFlags::Sharpening) && effects.sharpening.strength > 1e-3)
					{
						ScreenSpaceSharpeningConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						cfg.inColor = texSource;
						cfg.outColor = texTarget;
						cfg.strength = effects.sharpening.strength;
						screenSpaceSharpening(cfg);
						std::swap(texSource, texTarget);
					}

					// blit to destination
					if (texSource != colorTexture)
					{
						const auto graphicsDebugScope = renderQueue->namedScope("effects blit");
						renderQueue->viewport(Vec2i(), resolution);
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
					renderQueue->viewport(Vec2i(), resolution);
					renderQueue->bind(target, 0); // ensure the texture is properly initialized
					renderQueue->bind(colorTexture, 0);
					renderQueue->bind(shaderBlitPixels);
					renderQueue->bind(renderTarget);
					renderQueue->colorTexture(renderTarget, 0, target);
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

			Holder<AsyncTask> prepareShadowmap(Entity *e, const LightComponent &lc, const ShadowmapComponent &sc)
			{
				CAGE_ASSERT(e->id() != 0); // lights with shadowmap may not be anonymous

				Holder<RenderPipelineImpl> data = systemMemory().createHolder<RenderPipelineImpl>(this);
				this->data->shadowmaps[e] = data.share();

				data->name = Stringizer() + name + "_shadowmap_" + e->id();
				data->shadowmap = ShadowmapData();
				ShadowmapData &shadowmap = *data->shadowmap;
				shadowmap.lightComponent = lc;
				shadowmap.shadowmapComponent = sc;
				data->resolution = Vec2i(sc.resolution);
				data->model = modelTransform(e);
				data->transform = Transform(Vec3(data->model * Vec4(0, 0, 0, 1)));
				data->view = inverse(data->model);
				data->projection = [&]()
				{
					switch (lc.lightType)
					{
						case LightTypeEnum::Directional:
							return orthographicProjection(-sc.directionalWorldSize, sc.directionalWorldSize, -sc.directionalWorldSize, sc.directionalWorldSize, -sc.directionalWorldSize, sc.directionalWorldSize);
						case LightTypeEnum::Spot:
							return perspectiveProjection(lc.spotAngle, 1, lc.minDistance, lc.maxDistance);
						case LightTypeEnum::Point:
							return perspectiveProjection(Degs(90), 1, lc.minDistance, lc.maxDistance);
						default:
							CAGE_THROW_CRITICAL(Exception, "invalid light type");
					}
				}();
				data->viewProj = data->projection * data->view;
				data->frustum = Frustum(data->viewProj);
				static constexpr Mat4 bias = Mat4(0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.5, 0.5, 1.0);
				shadowmap.shadowUni.shadowMat = bias * data->viewProj;

				{
					const String name = Stringizer() + data->name + "_" + data->resolution;
					shadowmap.shadowTexture = provisionalGraphics->texture(name, shadowmap.lightComponent.lightType == LightTypeEnum::Point ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D,
						[resolution = data->resolution, format = shadowmap.lightComponent.lightType == LightTypeEnum::Point ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT24](Texture *t)
						{
							t->initialize(resolution, 1, format);
							t->filters(GL_LINEAR, GL_LINEAR, 16);
							t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
						});
				}

				{
					UniShadowedLight &uni = shadowmap.shadowUni;
					(UniLight &)uni = initializeLightUni(data->model, lc, transform.position);
					uni.shadowParams[2] = shadowmap.shadowmapComponent.normalOffsetScale;
					uni.shadowParams[3] = shadowmap.shadowmapComponent.shadowFactor;
					uni.params[0] += 1; // shadowed light type
				}

				return tasksRunAsync<RenderPipelineImpl>("render shadowmap task", [](RenderPipelineImpl &impl, uint32) { impl.taskShadowmap(); }, std::move(data));
			}

			Holder<RenderQueue> prepareCamera()
			{
				CAGE_ASSERT(!name.empty());
				data = CameraData();

				{
					std::vector<Holder<AsyncTask>> tasks;
					entitiesVisitor(
						[&](Entity *e, const LightComponent &lc, const ShadowmapComponent &sc)
						{
							if ((lc.sceneMask & camera.sceneMask) == 0)
								return;
							tasks.push_back(prepareShadowmap(e, lc, sc));
						},
						+scene, false);
					tasks.push_back(tasksRunAsync("render camera task", Delegate<void(uint32)>().bind<RenderPipelineImpl, &RenderPipelineImpl::taskCamera>(this)));
					for (const auto &it : tasks)
						it->wait();
				}

				Holder<RenderQueue> queue = newRenderQueue(name + "_pipeline", provisionalGraphics);

				{
					// viewport must be dispatched before shadowmaps
					UniViewport viewport;
					viewport.vMat = view;
					viewport.pMat = projection;
					viewport.vpMat = viewProj;
					viewport.vpInv = inverse(viewProj);
					viewport.eyePos = model * Vec4(0, 0, 0, 1);
					viewport.eyeDir = model * Vec4(0, 0, -1, 0);
					viewport.ambientLight = Vec4(colorGammaToLinear(camera.ambientColor) * camera.ambientIntensity, 0);
					viewport.viewport = Vec4(Vec2(), Vec2(resolution));
					viewport.time = Vec4(frameIndex % 10000, (currentTime % uint64(1e6)) / 1e6, (currentTime % uint64(1e9)) / 1e9, 0);
					queue->universalUniformStruct(viewport, CAGE_SHADER_UNIBLOCK_VIEWPORT);
				}

				// ensure that shadowmaps are rendered before the camera
				for (auto &shm : data->shadowmaps)
					queue->enqueue(std::move(shm.second->renderQueue));

				queue->enqueue(std::move(renderQueue));
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
