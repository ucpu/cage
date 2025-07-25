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
		extern uint32 GuiTextFontDefault;
	}

	namespace
	{
		const ConfigBool confGlobalRenderMissingModels("cage/graphics/renderMissingModels", false);
		const ConfigBool confGlobalRenderSkeletonBones("cage/graphics/renderSkeletonBones", false);

		struct UniViewport
		{
			// this matrix is same for shadowmaps and for the camera
			Mat4 viewMat; // camera view matrix
			Vec4 eyePos;
			Vec4 eyeDir;
			Vec4 viewport; // x, y, w, h
			Vec4 ambientLight; // linear rgb, unused
			Vec4 skyLight; // linear rgb, unused
			Vec4 time; // frame index (loops at 10000), time (loops every second), time (loops every 1000 seconds), unused
		};

		struct UniProjection
		{
			// these matrices are different for each shadowmap and the actual camera
			Mat4 viewMat;
			Mat4 projMat;
			Mat4 vpMat;
			Mat4 vpInv;
		};

		struct UniMesh
		{
			Mat3x4 modelMat;
			Vec4 color; // linear rgb (NOT alpha-premultiplied), opacity
			Vec4 animation; // time (seconds), speed, offset (normalized), unused
		};

		struct UniLight
		{
			Vec4 color; // linear rgb, intensity
			Vec4 position; // xyz, sortingIntensity
			Vec4 direction; // xyz, sortingPriority
			Vec4 attenuation; // attenuationType, minDistance, maxDistance, unused
			Vec4 params; // lightType, ssaoFactor, spotAngle, spotExponent
		};

		struct UniShadowedLight : public UniLight
		{
			// UniLight is inherited
			Mat4 shadowMat[CAGE_SHADER_MAX_SHADOWMAPSCASCADES];
			Vec4 shadowParams; // shadowmapSamplerIndex, unused, normalOffsetScale, shadowFactor
			Vec4 cascadesDepths;
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
			Holder<SkeletalAnimationPreparatorInstance> skeletalAnimation;
			Holder<Model> mesh;
		};

		struct RenderIcon : private Noncopyable
		{
			Holder<Texture> texture;
			Holder<Model> mesh;
		};

		struct RenderText : private Noncopyable
		{
			FontLayoutResult layout;
			Holder<Font> font;
		};

		struct RenderData : private Noncopyable
		{
			std::variant<std::monostate, RenderModel, RenderIcon, RenderText> data;
			Mat4 model;
			Vec4 color = Vec4::Nan(); // linear rgb (NOT alpha-premultiplied), opacity
			Vec4 animation = Vec4::Nan(); // time (seconds), speed, offset (normalized), unused
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
					[this](const auto &r) -> std::tuple<sint32, bool, Real, void *, void *, bool>
					{
						using T = std::decay_t<decltype(r)>;
						if constexpr (std::is_same_v<T, RenderModel>)
							return { renderLayer, translucent, depth, +r.mesh, nullptr, !!r.skeletalAnimation };
						if constexpr (std::is_same_v<T, RenderIcon>)
							return { renderLayer, translucent, depth, +r.mesh, +r.texture, false };
						if constexpr (std::is_same_v<T, RenderText>)
							return { renderLayer, translucent, depth, +r.font, nullptr, false };
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
			std::vector<Holder<struct RenderPipelineImpl>> shadowmaps;
			uint32 lightsCount = 0;
			uint32 shadowedLightsCount = 0;
		};

		struct ShadowmapData : private Noncopyable
		{
			UniShadowedLight shadowUni;
			Holder<ProvisionalTexture> shadowTexture;
			LightComponent lightComponent;
			ShadowmapComponent shadowmapComponent;
			uint32 cascade = 0;
		};

		struct RenderPipelineImpl : public RenderPipelineConfig
		{
			Holder<Model> modelSquare, modelBone, modelIcon;
			Holder<ShaderProgram> shaderBlitPixels;
			Holder<ShaderProgram> shaderDepth, shaderDepthCutOut;
			Holder<ShaderProgram> shaderStandard, shaderStandardCutOut;
			Holder<ShaderProgram> shaderIcon, shaderIconCutOut, shaderIconAnimated, shaderIconCutOutAnimated;
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
			bool cnfRenderMissingModels = confGlobalRenderMissingModels;
			bool cnfRenderSkeletonBones = confGlobalRenderSkeletonBones;

			// create pipeline for regular camera
			explicit RenderPipelineImpl(const RenderPipelineConfig &config) : RenderPipelineConfig(config)
			{
				if (!assets->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/cage.pack")) || !assets->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/shaders/engine/engine.pack")))
					return;

				modelSquare = assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/square.obj"));
				modelBone = assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/bone.obj"));
				modelIcon = assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/icon.obj"));
				CAGE_ASSERT(modelSquare && modelBone && modelIcon);

				shaderBlitPixels = assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/engine/blitPixels.glsl"))->get(0);
				CAGE_ASSERT(shaderBlitPixels);

				Holder<MultiShaderProgram> standard = assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/engine/standard.glsl"));
				CAGE_ASSERT(standard);
				shaderDepth = standard->get(HashString("DepthOnly"));
				shaderDepthCutOut = standard->get(HashString("DepthOnly") + HashString("CutOut"));
				shaderStandard = standard->get(0);
				shaderStandardCutOut = standard->get(HashString("CutOut"));

				Holder<MultiShaderProgram> icon = assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/engine/icon.glsl"));
				CAGE_ASSERT(icon);
				shaderIcon = icon->get(0);
				shaderIconCutOut = icon->get(HashString("CutOut"));
				shaderIconAnimated = icon->get(HashString("Animated"));
				shaderIconCutOutAnimated = icon->get(HashString("CutOut") + HashString("Animated"));

				shaderText = assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shaders/engine/text.glsl"))->get(0);
				CAGE_ASSERT(shaderText);

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
				SHARE(modelIcon);
				SHARE(shaderBlitPixels);
				SHARE(shaderDepth);
				SHARE(shaderDepthCutOut);
				SHARE(shaderStandard);
				SHARE(shaderStandardCutOut);
				SHARE(shaderIcon);
				SHARE(shaderIconCutOut);
				SHARE(shaderIconAnimated);
				SHARE(shaderIconCutOutAnimated);
				SHARE(shaderText);
				SHARE(skeletonPreparatorCollection);
#undef SHARE

				transformComponent = camera->transformComponent;
				prevTransformComponent = camera->prevTransformComponent;
			}

			Transform modelTransform(Entity *e) const
			{
				CAGE_ASSERT(e->has(transformComponent));
				if (e->has(prevTransformComponent))
				{
					const Transform c = e->value<TransformComponent>(transformComponent);
					const Transform p = e->value<TransformComponent>(prevTransformComponent);
					return interpolate(p, c, interpolationFactor);
				}
				else
					return e->value<TransformComponent>(transformComponent);
			}

			UniMesh makeMeshUni(const RenderData &rd) const
			{
				UniMesh uni;
				uni.modelMat = Mat3x4(rd.model);
				uni.color = rd.color;
				uni.animation = rd.animation;
				return uni;
			}

			static Vec4 initializeColor(const ColorComponent &cc)
			{
				const Vec3 c = valid(cc.color) ? colorGammaToLinear(cc.color) : Vec3(1);
				const Real i = valid(cc.intensity) ? cc.intensity : 1;
				const Real o = valid(cc.opacity) ? cc.opacity : 1;
				return Vec4(c * i, o);
			}

			static UniLight initializeLightUni(const Mat4 &model, const LightComponent &lc, const ColorComponent &cc, const Vec3 &cameraCenter)
			{
				CAGE_ASSERT(lc.minDistance > 0 && lc.maxDistance > lc.minDistance);
				static constexpr Real RoundingOffset = 1e-7;
				UniLight uni;

				uni.color = Vec4(colorGammaToLinear(valid(cc.color) ? cc.color : Vec3(1)), valid(cc.intensity) ? cc.intensity : 1);
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
				uni.direction[3] = lc.priority;

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
				std::sort(lights.begin(), lights.end(), [&](const UniLight &a, const UniLight &b) { return std::pair(a.direction[3], a.position[3]) > std::pair(b.direction[3], b.position[3]); });
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

			CAGE_FORCE_INLINE void appendShaderCustomData(Entity *e, uint32 customDataCount) const
			{
				if (!customDataCount)
					return;
				if (e && e->has<ShaderDataComponent>())
				{
					const auto sub = subRange(PointerRange<const Real>(e->value<ShaderDataComponent>().matrix.data).cast<const float>(), 0, customDataCount);
					for (const auto it : sub)
						uniCustomData.push_back(it);
				}
				else
				{
					for (uint32 i = 0; i < customDataCount; i++)
						uniCustomData.push_back({});
				}
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
						appendShaderCustomData(inst.e, shader->customDataCount);
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

			void renderIcons(const RenderModeEnum renderMode, const PointerRange<const RenderData> instances) const
			{
				CAGE_ASSERT(!instances.empty());
				const RenderData &rd = instances[0];
				CAGE_ASSERT(std::holds_alternative<RenderIcon>(rd.data));

				if (renderMode != RenderModeEnum::Color)
					return;

				const Holder<Model> mesh = std::get<RenderIcon>(rd.data).mesh.share();
				const Holder<Texture> texture = std::get<RenderIcon>(rd.data).texture.share();

				Holder<ShaderProgram> shader = [&]()
				{
					if (mesh->shaderName)
					{
						uint32 variant = 0;
						if (any(mesh->flags & MeshRenderFlags::CutOut))
							variant += HashString("CutOut");
						return assets->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(mesh->shaderName)->get(variant);
					}
					else
					{
						if (texture->target() == GL_TEXTURE_2D_ARRAY)
						{
							if (any(mesh->flags & MeshRenderFlags::CutOut))
								return shaderIconCutOutAnimated.share();
							else
								return shaderIconAnimated.share();
						}
						else
						{
							if (any(mesh->flags & MeshRenderFlags::CutOut))
								return shaderIconCutOut.share();
							else
								return shaderIcon.share();
						}
					}
				}();
				renderQueue->bind(shader);

				renderQueue->culling(none(mesh->flags & MeshRenderFlags::TwoSided));
				renderQueue->depthTest(any(mesh->flags & MeshRenderFlags::DepthTest));
				renderQueue->depthWrite(any(mesh->flags & MeshRenderFlags::DepthWrite));
				renderQueue->depthFuncLessEqual();
				renderQueue->blending(rd.translucent);
				if (rd.translucent)
					renderQueue->blendFuncAlphaTransparency();

				switch (texture->target())
				{
					case GL_TEXTURE_2D_ARRAY:
						renderQueue->bind(texture, CAGE_SHADER_TEXTURE_ALBEDO_ARRAY);
						break;
					case GL_TEXTURE_CUBE_MAP:
						renderQueue->bind(texture, CAGE_SHADER_TEXTURE_ALBEDO_CUBE);
						break;
					default:
						renderQueue->bind(texture, CAGE_SHADER_TEXTURE_ALBEDO);
						break;
				}

				// separate instances into draw calls by the limit
				const uint32 limit = CAGE_SHADER_MAX_MESHES;
				for (uint32 offset = 0; offset < instances.size(); offset += limit)
				{
					const uint32 count = min(limit, numeric_cast<uint32>(instances.size()) - offset);
					uniMeshes.clear();
					uniMeshes.reserve(count);
					CAGE_ASSERT(shader->customDataCount <= 16);
					uniCustomData.clear();
					if (shader->customDataCount)
						uniCustomData.reserve(shader->customDataCount * count);

					for (const auto &inst : subRange(instances, offset, count))
					{
						CAGE_ASSERT(std::holds_alternative<RenderIcon>(inst.data));
						CAGE_ASSERT(+std::get<RenderIcon>(inst.data).mesh == +mesh);
						CAGE_ASSERT(+std::get<RenderIcon>(inst.data).texture == +texture);
						uniMeshes.push_back(makeMeshUni(inst));
						appendShaderCustomData(inst.e, shader->customDataCount);
					}

					renderQueue->universalUniformArray<const UniMesh>(uniMeshes, CAGE_SHADER_UNIBLOCK_MESHES);
					if (!uniCustomData.empty())
						renderQueue->universalUniformArray<const float>(uniCustomData, CAGE_SHADER_UNIBLOCK_CUSTOMDATA);
					renderQueue->draw(mesh, count);
				}

				renderQueue->checkGlErrorDebug();
			}

			void renderTexts(const RenderModeEnum renderMode, const PointerRange<const RenderData> instances) const
			{
				CAGE_ASSERT(!instances.empty());
				const RenderData &rd = instances[0];
				CAGE_ASSERT(std::holds_alternative<RenderText>(rd.data));

				if (renderMode != RenderModeEnum::Color)
					return;

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
						if constexpr (std::is_same_v<T, RenderIcon>)
							renderIcons(renderMode, instances);
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
						[&](const auto &r) -> std::tuple<void *, void *, bool, bool>
						{
							using T = std::decay_t<decltype(r)>;
							if constexpr (std::is_same_v<T, RenderModel>)
								return { +r.mesh, nullptr, !!r.skeletalAnimation, d.translucent };
							if constexpr (std::is_same_v<T, RenderIcon>)
								return { +r.mesh, +r.texture, false, d.translucent };
							if constexpr (std::is_same_v<T, RenderText>)
								return { +r.font, nullptr, false, d.translucent };
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

				std::optional<SkeletalAnimationComponent> ps;
				if (rd.e->has<SkeletalAnimationComponent>())
					ps = rd.e->value<SkeletalAnimationComponent>();

				ModelComponent render = rd.e->value<ModelComponent>();
				ColorComponent color = rd.e->getOrDefault<ColorComponent>();
				AnimationSpeedComponent anim = rd.e->getOrDefault<AnimationSpeedComponent>();
				const uint64 startTime = rd.e->getOrDefault<SpawnTimeComponent>().spawnTime;

				if (parent)
				{
					if (!color.color.valid())
						color.color = parent->color;
					if (!color.intensity.valid())
						color.intensity = parent->intensity;
					if (!color.opacity.valid())
						color.opacity = parent->opacity;
					if (!ps && parent->skelAnimId)
						ps = SkeletalAnimationComponent();
					if (ps && !ps->animation)
						ps->animation = parent->skelAnimId;
				}

				if (!anim.speed.valid())
					anim.speed = 1;
				if (!anim.offset.valid())
					anim.offset = 0;

				if (!rm.mesh->bones)
					ps.reset();
				if (ps)
				{
					if (Holder<SkeletalAnimation> a = assets->get<AssetSchemeIndexSkeletalAnimation, SkeletalAnimation>(ps->animation))
					{
						const Real coefficient = detail::evalCoefficientForSkeletalAnimation(+a, currentTime, startTime, anim.speed, anim.offset);
						rm.skeletalAnimation = skeletonPreparatorCollection->create(rd.e, std::move(a), coefficient, rm.mesh->importTransform, cnfRenderSkeletonBones);
						rm.skeletalAnimation->prepare();
					}
					else
						ps.reset();
				}

				rd.color = initializeColor(color);
				rd.animation = Vec4((double)(sint64)(currentTime - startTime) / (double)1'000'000, anim.speed, anim.offset, 0);
				rd.renderLayer = render.renderLayer + rm.mesh->layer;
				rd.translucent = any(rm.mesh->flags & (MeshRenderFlags::Transparent | MeshRenderFlags::Fade)) || rd.color[3] < 1;
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

			void prepareIcon(Entity *e, const IconComponent &ic)
			{
				CAGE_ASSERT(ic.icon != 0 && ic.icon != m);
				RenderIcon ri;
				ri.texture = assets->get<AssetSchemeIndexTexture, Texture>(ic.icon);
				if (!ri.texture)
					return;
				ri.mesh = ic.model ? assets->get<AssetSchemeIndexModel, Model>(ic.model) : modelIcon.share();
				if (!ri.mesh)
					return;
				CAGE_ASSERT(ri.mesh->bones == 0);
				RenderData rd;
				rd.e = e;
				rd.model = Mat4(modelTransform(e));
				rd.color = initializeColor(e->getOrDefault<ColorComponent>());
				const uint64 startTime = rd.e->getOrDefault<SpawnTimeComponent>().spawnTime;
				rd.animation = Vec4((double)(sint64)(currentTime - startTime) / (double)1'000'000, 1, 0, 0);
				rd.renderLayer = ic.renderLayer + ri.mesh->layer;
				rd.translucent = true;
				rd.depth = (viewProj * (rd.model * Vec4(0, 0, 0, 1)))[2] * -1;
				rd.data = std::move(ri);
				renderData.push_back(std::move(rd));
			}

			void prepareText(Entity *e, TextComponent tc)
			{
				if (!tc.font)
					tc.font = detail::GuiTextFontDefault;
				if (!tc.font)
					tc.font = HashString("cage/fonts/ubuntu/regular.ttf");
				RenderText rt;
				rt.font = assets->get<AssetSchemeIndexFont, Font>(tc.font);
				if (!rt.font)
					return;
				FontFormat format;
				format.size = 1;
				format.align = tc.align;
				format.lineSpacing = tc.lineSpacing;
				const String str = textsGet(tc.textId, e->getOrDefault<TextValueComponent>().value);
				rt.layout = rt.font->layout(str, format);
				if (rt.layout.glyphs.empty())
					return;
				RenderData rd;
				rd.e = e;
				rd.model = Mat4(modelTransform(e)) * Mat4(Vec3(rt.layout.size * Vec2(-0.5, 0.5), 0));
				rd.color = initializeColor(e->getOrDefault<ColorComponent>());
				rd.renderLayer = tc.renderLayer;
				rd.translucent = true;
				rd.depth = (viewProj * (rd.model * Vec4(0, 0, 0, 1)))[2] * -1;
				rd.data = std::move(rt);
				renderData.push_back(std::move(rd));
			}

			bool failedMask(Entity *e) const
			{
				const uint32 c = e->getOrDefault<SceneComponent>().sceneMask;
				return (c & cameraSceneMask) == 0;
			}

			void prepareEntities()
			{
				ProfilingScope profiling("prepare entities");
				profiling.set(Stringizer() + "entities: " + scene->count());

				renderData.reserve(scene->component<ModelComponent>()->count() + scene->component<IconComponent>()->count() + scene->component<TextComponent>()->count());

				entitiesVisitor(
					[&](Entity *e, const ModelComponent &rc)
					{
						if (!rc.model)
							return;
						if (failedMask(e))
							return;
						RenderData rd;
						rd.e = e;
						rd.model = Mat4(modelTransform(e));
						if (Holder<RenderObject> obj = assets->get<AssetSchemeIndexRenderObject, RenderObject>(rc.model))
						{
							prepareObject(rd, std::move(obj));
							return;
						}
						if (Holder<Model> mesh = assets->get<AssetSchemeIndexModel, Model>(rc.model))
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
							rm.mesh = assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/fake.obj"));
							rd.data = std::move(rm);
							prepareModel(rd);
							return;
						}
					},
					+scene, false);

				entitiesVisitor(
					[&](Entity *e, const IconComponent &ic)
					{
						if (!ic.icon)
							return;
						if (failedMask(e))
							return;
						prepareIcon(e, ic);
					},
					+scene, false);

				entitiesVisitor(
					[&](Entity *e, const TextComponent &tc)
					{
						if (failedMask(e))
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

			void prepareProjection()
			{
				UniProjection uni;
				uni.viewMat = view;
				uni.projMat = projection;
				uni.vpMat = viewProj;
				uni.vpInv = inverse(viewProj);
				renderQueue->universalUniformStruct(uni, CAGE_SHADER_UNIBLOCK_PROJECTION);
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
							if (failedMask(e))
								return;
							if (e->has<ShadowmapComponent>())
								return;
							UniLight uni = initializeLightUni(Mat4(modelTransform(e)), lc, e->getOrDefault<ColorComponent>(), transform.position);
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
					uint32 tex2dCount = 0, texCubeCount = 0;
					shadows.reserve(CAGE_SHADER_MAX_SHADOWMAPSCUBE + CAGE_SHADER_MAX_SHADOWMAPS2D);
					for (auto &sh_ : data->shadowmaps)
					{
						CAGE_ASSERT(sh_->shadowmap);
						ShadowmapData &sh = *sh_->shadowmap;
						if (sh.cascade != 0)
							continue;
						if (sh.lightComponent.lightType == LightTypeEnum::Point)
						{
							if (texCubeCount == CAGE_SHADER_MAX_SHADOWMAPSCUBE)
								CAGE_THROW_ERROR(Exception, "too many shadowmaps (cube shadows)");
							renderQueue->bind(sh.shadowTexture, CAGE_SHADER_TEXTURE_SHADOWMAPCUBE0 + texCubeCount);
							sh.shadowUni.shadowParams[0] = texCubeCount;
							texCubeCount++;
						}
						else
						{
							if (tex2dCount == CAGE_SHADER_MAX_SHADOWMAPS2D)
								CAGE_THROW_ERROR(Exception, "too many shadowmaps (2D shadows)");
							renderQueue->bind(sh.shadowTexture, CAGE_SHADER_TEXTURE_SHADOWMAP2D0 + tex2dCount);
							sh.shadowUni.shadowParams[0] = tex2dCount;
							tex2dCount++;
						}
						shadows.push_back(sh.shadowUni);
					}
					CAGE_ASSERT(shadows.size() == tex2dCount + texCubeCount);
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
				prepareProjection();
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

				// ssao
				if (any(effects.effects & ScreenSpaceEffectsFlags::AmbientOcclusion))
				{
					ScreenSpaceAmbientOcclusionConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					(ScreenSpaceAmbientOcclusion &)cfg = effects.ssao;
					cfg.proj = projection;
					cfg.inDepth = depthTexture;
					cfg.frameIndex = frameIndex;
					screenSpaceAmbientOcclusion(cfg);

					// bind the texture for sampling
					renderQueue->bind(cfg.outAo, CAGE_SHADER_TEXTURE_SSAO);
				}

				// standard pass target
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

			Holder<RenderQueue> prepareCamera()
			{
				CAGE_ASSERT(!name.empty());
				data = CameraData();

				{
					std::vector<Holder<AsyncTask>> tasks;
					entitiesVisitor(
						[&](Entity *e, const LightComponent &lc, const ShadowmapComponent &sc)
						{
							if (failedMask(e))
								return;
							prepareShadowmap(tasks, e, lc, sc);
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
					viewport.viewMat = view;
					viewport.eyePos = model * Vec4(0, 0, 0, 1);
					viewport.eyeDir = model * Vec4(0, 0, -1, 0);
					viewport.ambientLight = Vec4(colorGammaToLinear(camera.ambientColor) * camera.ambientIntensity, 0);
					viewport.skyLight = Vec4(colorGammaToLinear(camera.skyColor) * camera.skyIntensity, 0);
					viewport.viewport = Vec4(Vec2(), Vec2(resolution));
					viewport.time = Vec4(frameIndex % 10'000, (currentTime % uint64(1e6)) / 1e6, (currentTime % uint64(1e9)) / 1e9, 0);
					queue->universalUniformStruct(viewport, CAGE_SHADER_UNIBLOCK_VIEWPORT);
				}

				// ensure that shadowmaps are rendered before the camera
				for (auto &shm : data->shadowmaps)
					queue->enqueue(std::move(shm->renderQueue));

				queue->enqueue(std::move(renderQueue));
				return queue;
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
				renderQueue->depthTexture(renderTarget, shadowmap->shadowTexture, shadowmap->cascade);
				renderQueue->activeAttachments(renderTarget, 0);
				renderQueue->checkFrameBuffer(renderTarget);
				renderQueue->viewport(Vec2i(), resolution);
				renderQueue->colorWrite(false);
				renderQueue->clear(false, true);
				renderQueue->checkGlErrorDebug();

				prepareProjection();
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

			Holder<RenderPipelineImpl> createShadowmapPipeline(Entity *e, const LightComponent &lc, const ShadowmapComponent &sc)
			{
				CAGE_ASSERT(e->id() != 0); // lights with shadowmap may not be anonymous

				Holder<RenderPipelineImpl> data = systemMemory().createHolder<RenderPipelineImpl>(this);
				this->data->shadowmaps.push_back(data.share());

				data->name = Stringizer() + name + "_shadowmap_" + e->id();
				data->shadowmap = ShadowmapData();
				ShadowmapData &shadowmap = *data->shadowmap;
				shadowmap.lightComponent = lc;
				shadowmap.shadowmapComponent = sc;
				data->resolution = Vec2i(sc.resolution);

				{ // quantize light direction - reduces shimmering of slowly rotating lights
					Transform src = modelTransform(e);
					Vec3 f = src.orientation * Vec3(0, 0, -1);
					f *= 1'000;
					for (uint32 i = 0; i < 3; i++)
						f[i] = round(f[i]);
					f = normalize(f);
					src.orientation = Quat(f, src.orientation * Vec3(0, 1, 0));
					data->model = Mat4(src);
				}

				{
					UniShadowedLight &uni = shadowmap.shadowUni;
					(UniLight &)uni = initializeLightUni(data->model, lc, e->getOrDefault<ColorComponent>(), transform.position);
					uni.shadowParams[2] = sc.normalOffsetScale;
					uni.shadowParams[3] = sc.shadowFactor;
					uni.params[0] += 1; // shadowed light type
				}

				return data;
			}

			void initializeShadowmapCascade()
			{
				CAGE_ASSERT(shadowmap->lightComponent.lightType == LightTypeEnum::Directional);
				CAGE_ASSERT(camera.shadowmapFrustumDepthFraction > 0 && camera.shadowmapFrustumDepthFraction <= 1);

				const auto sc = shadowmap->shadowmapComponent;
				CAGE_ASSERT(sc.cascadesSplitLogFactor >= 0 && sc.cascadesSplitLogFactor <= 1);
				CAGE_ASSERT(sc.cascadesCount > 0 && sc.cascadesCount <= CAGE_SHADER_MAX_SHADOWMAPSCASCADES);
				CAGE_ASSERT(sc.cascadesPaddingDistance >= 0);

				const Mat4 invP = inverse(projection);
				const auto &getPlane = [&](Real ndcZ) -> Real
				{
					const Vec4 a = invP * Vec4(0, 0, ndcZ, 1);
					return -a[2] / a[3];
				};
				const Real cameraNear = getPlane(-1);
				const Real cameraFar = getPlane(1) * camera.shadowmapFrustumDepthFraction;
				CAGE_ASSERT(cameraNear > 0 && cameraFar > cameraNear);

				const auto &getSplit = [&](Real f) -> Real
				{
					const Real uniform = cameraNear + (cameraFar - cameraNear) * f;
					const Real log = cameraNear * pow(cameraFar / cameraNear, f);
					return interpolate(uniform, log, sc.cascadesSplitLogFactor);
				};
				const Real splitNear = getSplit(Real(shadowmap->cascade + 0) / sc.cascadesCount);
				const Real splitFar = getSplit(Real(shadowmap->cascade + 1) / sc.cascadesCount);
				CAGE_ASSERT(splitNear > 0 && splitFar > splitNear);

				const auto viewZtoNdcZ = [&](Real viewZ) -> Real
				{
					const Vec4 a = projection * Vec4(0, 0, -viewZ, 1);
					return a[2] / a[3];
				};
				const Real splitNearNdc = viewZtoNdcZ(splitNear);
				const Real splitFarNdc = viewZtoNdcZ(splitFar);

				const Mat4 invVP = inverse(projection * Mat4(inverse(transform)));
				const auto &getPoint = [&](Vec3 p) -> Vec3
				{
					const Vec4 a = invVP * Vec4(p, 1);
					return Vec3(a) / a[3];
				};
				// world-space corners of the camera frustum slice
				const std::array<Vec3, 8> corners = { getPoint(Vec3(-1, -1, splitNearNdc)), getPoint(Vec3(1, -1, splitNearNdc)), getPoint(Vec3(1, 1, splitNearNdc)), getPoint(Vec3(-1, 1, splitNearNdc)), getPoint(Vec3(-1, -1, splitFarNdc)), getPoint(Vec3(1, -1, splitFarNdc)), getPoint(Vec3(1, 1, splitFarNdc)), getPoint(Vec3(-1, 1, splitFarNdc)) };

				const Vec3 lightDir = Vec3(model * Vec4(0, 0, -1, 0));
				const Vec3 lightUp = abs(dot(lightDir, Vec3(0, 1, 0))) > 0.99 ? Vec3(0, 0, 1) : Vec3(0, 1, 0);

#if (0)

				Sphere sph = makeSphere(corners);
				view = Mat4(inverse(Transform(sph.center, Quat(lightDir, lightUp))));

				{ // stabilization
					const Real texelSize = sph.radius * 2 / sc.resolution;
					Vec3 snapOrigin = Vec3(view * Vec4(sph.center, 1));
					snapOrigin /= texelSize;
					for (uint32 i = 0; i < 3; i++)
						snapOrigin[i] = floor(snapOrigin[i]);
					snapOrigin *= texelSize;
					sph.center = Vec3(inverse(view) * Vec4(snapOrigin, 1));
					view = Mat4(inverse(Transform(sph.center, Quat(lightDir, lightUp))));
				}

				Aabb shadowBox = Aabb(Sphere(Vec3(), sph.radius));

#else

				Aabb worldBox;
				for (Vec3 wp : corners)
					worldBox += Aabb(wp);

				Vec3 eye = worldBox.center();
				view = Mat4(inverse(Transform(eye, Quat(lightDir, lightUp))));

				Aabb shadowBox;
				for (Vec3 wp : corners)
					shadowBox += Aabb(Vec3(view * Vec4(wp, 1)));

				{ // stabilization
					const Real texelSize = (shadowBox.b[0] - shadowBox.a[0]) / sc.resolution;
					Vec3 snapOrigin = Vec3(view * Vec4(worldBox.center(), 1));
					snapOrigin /= texelSize;
					for (uint32 i = 0; i < 3; i++)
						snapOrigin[i] = floor(snapOrigin[i]);
					snapOrigin *= texelSize;
					eye = Vec3(inverse(view) * Vec4(snapOrigin, 1));
					view = Mat4(inverse(Transform(eye, Quat(lightDir, lightUp))));

					shadowBox = Aabb();
					for (Vec3 wp : corners)
						shadowBox += Aabb(Vec3(view * Vec4(wp, 1)));
				}

#endif

				// adjust the shadow near and far planes to not exclude preceding shadow casters
				shadowBox.a[2] -= sc.cascadesPaddingDistance;
				shadowBox.b[2] += sc.cascadesPaddingDistance;

				// move far plane much further, to improve precision
				const Real currentDepth = -shadowBox.a[2] + shadowBox.b[2];
				shadowBox.a[2] -= currentDepth * 10;

				projection = orthographicProjection(shadowBox.a[0], shadowBox.b[0], shadowBox.a[1], shadowBox.b[1], -shadowBox.b[2], -shadowBox.a[2]);

				viewProj = projection * view;
				frustum = Frustum(viewProj);
				static constexpr Mat4 bias = Mat4(0.5, 0, 0, 0, 0, 0.5, 0, 0, 0, 0, 0.5, 0, 0.5, 0.5, 0.5, 1);
				shadowmap->shadowUni.shadowMat[shadowmap->cascade] = bias * viewProj;
				shadowmap->shadowUni.cascadesDepths[shadowmap->cascade] = splitFar;
			}

			void initializeShadowmapSingle()
			{
				CAGE_ASSERT(shadowmap->cascade == 0);

				view = inverse(model);
				projection = [&]()
				{
					const auto lc = shadowmap->lightComponent;
					switch (lc.lightType)
					{
						case LightTypeEnum::Spot:
							return perspectiveProjection(lc.spotAngle, 1, lc.minDistance, lc.maxDistance);
						case LightTypeEnum::Point:
							return perspectiveProjection(Degs(90), 1, lc.minDistance, lc.maxDistance);
						case LightTypeEnum::Directional:
						default:
							CAGE_THROW_CRITICAL(Exception, "invalid light type");
					}
				}();

				viewProj = projection * view;
				frustum = Frustum(viewProj);
				static constexpr Mat4 bias = Mat4(0.5, 0, 0, 0, 0, 0.5, 0, 0, 0, 0, 0.5, 0, 0.5, 0.5, 0.5, 1);
				shadowmap->shadowUni.shadowMat[shadowmap->cascade] = bias * viewProj;
			}

			void prepareShadowmap(std::vector<Holder<AsyncTask>> &outputTasks, Entity *e, const LightComponent &lc, const ShadowmapComponent &sc)
			{
				switch (lc.lightType)
				{
					case LightTypeEnum::Directional:
					{
						Holder<ProvisionalTexture> tex;
						{
							const String name = Stringizer() + this->name + "_shadowmap_" + e->id() + "_cascades_" + sc.resolution;
							tex = provisionalGraphics->texture(name, GL_TEXTURE_2D_ARRAY,
								[resolution = sc.resolution](Texture *t)
								{
									t->initialize(Vec3i(resolution, resolution, CAGE_SHADER_MAX_SHADOWMAPSCASCADES), 1, GL_DEPTH_COMPONENT24);
									t->filters(GL_LINEAR, GL_LINEAR, 16);
									t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
								});
						}
						ShadowmapData *first = nullptr;
						for (uint32 cascade = 0; cascade < CAGE_SHADER_MAX_SHADOWMAPSCASCADES; cascade++)
						{
							auto data = createShadowmapPipeline(e, lc, sc);
							data->shadowmap->cascade = cascade;
							data->shadowmap->shadowTexture = tex.share();
							data->initializeShadowmapCascade();
							if (cascade == 0)
								first = &*data->shadowmap;
							else
							{
								first->shadowUni.cascadesDepths[cascade] = data->shadowmap->shadowUni.cascadesDepths[cascade];
								first->shadowUni.shadowMat[cascade] = data->shadowmap->shadowUni.shadowMat[cascade];
							}
							outputTasks.push_back(tasksRunAsync<RenderPipelineImpl>("render shadowmap cascade", [](RenderPipelineImpl &impl, uint32) { impl.taskShadowmap(); }, std::move(data)));
						}
						break;
					}
					case LightTypeEnum::Spot:
					case LightTypeEnum::Point:
					{
						auto data = createShadowmapPipeline(e, lc, sc);
						data->initializeShadowmapSingle();
						{
							const String name = Stringizer() + this->name + "_shadowmap_" + e->id() + "_" + sc.resolution;
							data->shadowmap->shadowTexture = provisionalGraphics->texture(name, data->shadowmap->lightComponent.lightType == LightTypeEnum::Point ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D_ARRAY,
								[resolution = data->resolution, internalFormat = data->shadowmap->lightComponent.lightType == LightTypeEnum::Point ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT24](Texture *t)
								{
									t->initialize(Vec3i(resolution, 1), 1, internalFormat);
									t->filters(GL_LINEAR, GL_LINEAR, 16);
									t->wraps(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
								});
						}
						outputTasks.push_back(tasksRunAsync<RenderPipelineImpl>("render shadowmap single", [](RenderPipelineImpl &impl, uint32) { impl.taskShadowmap(); }, std::move(data)));
						break;
					}
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid light type");
				}
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
