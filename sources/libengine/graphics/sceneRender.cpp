#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <variant>
#include <vector>

#include <cage-core/assetsManager.h>
#include <cage-core/assetsOnDemand.h>
#include <cage-core/assetsSchemes.h>
#include <cage-core/camera.h>
#include <cage-core/color.h>
#include <cage-core/config.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/geometry.h>
#include <cage-core/hashString.h>
#include <cage-core/meshIoCommon.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/profiling.h>
#include <cage-core/scopeGuard.h>
#include <cage-core/skeletalAnimation.h>
#include <cage-core/skeletalAnimationPreparator.h>
#include <cage-core/tasks.h>
#include <cage-core/texts.h>
#include <cage-engine/assetsSchemes.h>
#include <cage-engine/font.h>
#include <cage-engine/graphicsAggregateBuffer.h>
#include <cage-engine/graphicsBindings.h>
#include <cage-engine/graphicsBuffer.h>
#include <cage-engine/graphicsDevice.h>
#include <cage-engine/graphicsEncoder.h>
#include <cage-engine/model.h>
#include <cage-engine/renderObject.h>
#include <cage-engine/scene.h>
#include <cage-engine/sceneCustomDraw.h>
#include <cage-engine/sceneRender.h>
#include <cage-engine/screenSpaceEffects.h>
#include <cage-engine/shader.h>
#include <cage-engine/texture.h>
#include <cage-engine/window.h>

namespace cage
{
	struct AssetPack;

	namespace privat
	{
		Texture *getTextureDummy2d(GraphicsDevice *device);
		Texture *getTextureDummyArray(GraphicsDevice *device);
		Texture *getTextureDummyCube(GraphicsDevice *device);
		Texture *getTextureShadowsSampler(GraphicsDevice *device);
		GraphicsBuffer *getBufferDummy(GraphicsDevice *device);
	}

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
			Mat4 shadowMat[4];
			Vec4 shadowParams; // shadowmapSamplerIndex, unused, normalOffsetScale, shadowFactor
			Vec4 cascadesDepths;
		};

		struct UniOptions
		{
			Vec4i optsLights; // unshadowed lights count, shadowed lights count, enable ambient occlusion, enable normal map
			Vec4i optsSkeleton; // bones count, unused, unused, unused
		};

		struct UniMesh
		{
			Mat3x4 modelMat;
			Vec4 color; // linear rgb (NOT alpha-premultiplied), opacity
			Vec4 animation; // time (seconds), speed, offset (normalized), unused
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

		struct RenderCustom : private Noncopyable
		{};

		struct RenderData : private Noncopyable
		{
			std::variant<std::monostate, RenderModel, RenderIcon, RenderText, RenderCustom> data;
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
						if constexpr (std::is_same_v<T, RenderCustom>)
							return { renderLayer, translucent, depth, nullptr, nullptr, false };
						return {};
					},
					data);
			}
		};

		enum class RenderModeEnum
		{
			Shadowmap,
			DepthPrepass,
			Color,
		};

		struct CameraData : private Noncopyable
		{
			std::vector<Holder<struct SceneRenderImpl>> shadowmaps;
			uint32 lightsCount = 0;
			uint32 shadowedLightsCount = 0;
		};

		struct ShadowmapData : private Noncopyable
		{
			UniShadowedLight shadowUni;
			LightComponent lightComponent;
			ShadowmapComponent shadowmapComponent;
			Holder<Texture> shadowTexture;
			uint32 cascade = 0;
		};

		template<class T, class Mark, class Output>
		void partition(PointerRange<T> inputRange, Mark &&mark, Output &&output)
		{
			auto it = inputRange.begin();
			const auto et = inputRange.end();
			while (it != et)
			{
				const auto mk = mark(*it);
				auto i = it + 1;
				while (i != et && mark(*i) == mk)
					i++;
				PointerRange<T> instances = { &*it, &*i };
				output(instances);
				it = i;
			}
		}

		struct SceneRenderImpl : public SceneRenderConfig
		{
			Holder<Model> modelSquare, modelBone, modelIcon;
			Holder<Shader> shaderBlitPixels, shaderBlitScaled;
			Holder<MultiShader> shaderStandard, shaderIcon;
			Holder<Shader> shaderText;

			Holder<SkeletalAnimationPreparatorCollection> skeletonPreparatorCollection;
			EntityComponent *transformComponent = nullptr;
			EntityComponent *prevTransformComponent = nullptr;
			const bool cnfRenderMissingModels = confGlobalRenderMissingModels;
			const bool cnfRenderSkeletonBones = confGlobalRenderSkeletonBones;

			Mat4 model;
			Mat4 view;
			Mat4 viewProj;
			Frustum frustum;
			std::optional<CameraData> cameraData;
			std::optional<ShadowmapData> shadowmapData;
			Holder<GraphicsEncoder> encoder;
			Holder<GraphicsAggregateBuffer> aggregate;
			Holder<GraphicsBuffer> buffViewport, buffProjection, buffLights, buffShadowedLights;

			std::vector<RenderData> renderData;
			mutable std::vector<UniMesh> uniMeshes;
			mutable std::vector<Mat3x4> uniArmatures;
			mutable std::vector<float> uniCustomData;
			mutable std::vector<Holder<Model>> prepareModels;

			// create pipeline for regular camera
			explicit SceneRenderImpl(const SceneRenderConfig &config) : SceneRenderConfig(config)
			{
				if (!assets->get<AssetPack>(HashString("cage/cage.pack")) || !assets->get<AssetPack>(HashString("cage/shaders/engine/engine.pack")))
					return;

				modelSquare = assets->get<Model>(HashString("cage/models/square.obj"));
				modelBone = assets->get<Model>(HashString("cage/models/bone.obj"));
				modelIcon = assets->get<Model>(HashString("cage/models/icon.obj"));
				CAGE_ASSERT(modelSquare && modelBone && modelIcon);

				shaderBlitPixels = assets->get<MultiShader>(HashString("cage/shaders/engine/blitPixels.glsl"))->get(0);
				CAGE_ASSERT(shaderBlitPixels);

				shaderBlitScaled = assets->get<MultiShader>(HashString("cage/shaders/engine/blitScaled.glsl"))->get(0);
				CAGE_ASSERT(shaderBlitScaled);

				shaderStandard = assets->get<MultiShader>(HashString("cage/shaders/engine/standard.glsl"));
				CAGE_ASSERT(shaderStandard);

				shaderIcon = assets->get<MultiShader>(HashString("cage/shaders/engine/icon.glsl"));
				CAGE_ASSERT(shaderIcon);

				shaderText = assets->get<MultiShader>(HashString("cage/shaders/engine/text.glsl"))->get(0);
				CAGE_ASSERT(shaderText);

				transformComponent = scene->component<TransformComponent>();
				prevTransformComponent = scene->componentsByType(detail::typeIndex<TransformComponent>())[1];

				skeletonPreparatorCollection = newSkeletalAnimationPreparatorCollection(assets);
			}

			// create pipeline for shadowmap
			explicit SceneRenderImpl(const SceneRenderImpl *camera) : SceneRenderConfig(*camera)
			{
				CAGE_ASSERT(camera->skeletonPreparatorCollection);

#define SHARE(NAME) NAME = camera->NAME.share();
				SHARE(modelSquare);
				SHARE(modelBone);
				SHARE(modelIcon);
				SHARE(shaderBlitPixels);
				SHARE(shaderBlitScaled);
				SHARE(shaderStandard);
				SHARE(shaderIcon);
				SHARE(shaderText);
				SHARE(skeletonPreparatorCollection);
#undef SHARE

				transformComponent = camera->transformComponent;
				prevTransformComponent = camera->prevTransformComponent;

				buffViewport = camera->buffViewport.share();
			}

			CAGE_FORCE_INLINE Transform modelTransform(Entity *e) const
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

			CAGE_FORCE_INLINE UniMesh makeMeshUni(const RenderData &rd) const
			{
				UniMesh uni;
				uni.modelMat = Mat3x4(rd.model);
				uni.color = rd.color;
				uni.animation = rd.animation;
				return uni;
			}

			CAGE_FORCE_INLINE static Vec4 initializeColor(const ColorComponent &cc)
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
							return 11 + RoundingOffset;
						case LightTypeEnum::Spot:
							return 13 + RoundingOffset;
						case LightTypeEnum::Point:
							return 15 + RoundingOffset;
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
				if (lights.size() > limit)
					lights.resize(limit);

				// fade-out lights close to limit
				const uint32 s = max(limit * 85 / 100, 10u);
				const Real f = 1.0 / (limit - s + 1);
				for (uint32 i = s; i < lights.size(); i++)
					lights[i].color[3] *= saturate(1 - (i - s + 1) * f);
			}

			CAGE_FORCE_INLINE void appendShaderCustomData(Entity *e, uint32 customDataCount) const
			{
				if (!customDataCount)
					return;
				if (e && e->has<ShaderDataComponent>())
				{
					const auto wholeRange = PointerRange<const Real>(e->value<ShaderDataComponent>().matrix.data).cast<const float>();
					const auto sub = wholeRange.subRange(0, customDataCount);
					for (const auto it : sub)
						uniCustomData.push_back(it);
				}
				else
				{
					for (uint32 i = 0; i < customDataCount; i++)
						uniCustomData.push_back({});
				}
			}

			CAGE_FORCE_INLINE static uint32 textureShaderVariant(const TextureFlags flags)
			{
				if (flags == TextureFlags::None)
					return 0;
				if (any(flags & TextureFlags::Cubemap))
					return HashString("MaterialTexCube");
				if (any(flags & TextureFlags::Array))
					return HashString("MaterialTexArray");
				return HashString("MaterialTexRegular");
			}

			CAGE_FORCE_INLINE static Holder<Shader> pickShaderVariant(MultiShader *multiShader, const Model *model, uint32 textureVariant, const RenderModeEnum renderMode, const bool skeletalAnimation)
			{
				CAGE_ASSERT(multiShader);
				uint32 variant = 0;
				const auto &v = [&](uint32 c)
				{
					if (multiShader->checkKeyword(c))
						variant += c;
				};
				v(textureVariant);
				if (renderMode != RenderModeEnum::Color)
					v(HashString("DepthOnly"));
				if (any(model->renderFlags & MeshRenderFlags::CutOut))
					v(HashString("CutOut"));
				if (any(model->components & MeshComponentsFlags::Normals))
					v(HashString("Normals"));
				if (skeletalAnimation)
					v(HashString("SkeletalAnimation"));
				return multiShader->get(variant);
			}

			void renderModels(const RenderModeEnum renderMode, const PointerRange<const RenderData> instances) const
			{
				CAGE_ASSERT(!instances.empty());
				const RenderData &rd = instances[0];
				CAGE_ASSERT(std::holds_alternative<RenderModel>(rd.data));
				const RenderModel &rm = std::get<RenderModel>(rd.data);

				if (renderMode != RenderModeEnum::Color && rd.translucent)
					return;
				if (renderMode == RenderModeEnum::DepthPrepass && none(rm.mesh->renderFlags & MeshRenderFlags::DepthWrite))
					return;

				prepareModelBindings(device, assets, +rm.mesh); // todo remove

				Holder<MultiShader> multiShader = rm.mesh->shaderName ? assets->get<AssetSchemeIndexShader, MultiShader>(rm.mesh->shaderName) : shaderStandard.share();
				Holder<Shader> shader = pickShaderVariant(+multiShader, +rm.mesh, textureShaderVariant(rm.mesh->texturesFlags), renderMode, !!rm.skeletalAnimation);

				UniOptions uniOptions;
				{
					if (renderMode == RenderModeEnum::Color)
					{
						CAGE_ASSERT(cameraData);
						uniOptions.optsLights[0] = cameraData->lightsCount;
						uniOptions.optsLights[1] = cameraData->shadowedLightsCount;
					}
					const bool ssao = !rd.translucent && any(rm.mesh->renderFlags & MeshRenderFlags::DepthWrite) && any(effects.effects & ScreenSpaceEffectsFlags::AmbientOcclusion);
					uniOptions.optsLights[2] = ssao ? 1 : 0;
					uniOptions.optsLights[3] = !!rm.mesh->textureNames[2];
					uniOptions.optsSkeleton[0] = rm.mesh->bonesCount;
				}

				uniMeshes.clear();
				uniMeshes.reserve(instances.size());
				uniArmatures.clear();
				if (rm.skeletalAnimation)
					uniArmatures.reserve(rm.mesh->bonesCount * instances.size());
				CAGE_ASSERT(multiShader->customDataCount <= 16);
				uniCustomData.clear();
				if (multiShader->customDataCount)
					uniCustomData.reserve(multiShader->customDataCount * instances.size());

				for (const auto &inst : instances)
				{
					CAGE_ASSERT(std::holds_alternative<RenderModel>(inst.data));
					const RenderModel &r = std::get<RenderModel>(inst.data);
					CAGE_ASSERT(+r.mesh == +rm.mesh);
					uniMeshes.push_back(makeMeshUni(inst));
					if (rm.skeletalAnimation)
					{
						const auto a = r.skeletalAnimation->armature();
						CAGE_ASSERT(a.size() == rm.mesh->bonesCount);
						for (const auto &b : a)
							uniArmatures.push_back(b);
					}
					appendShaderCustomData(inst.e, multiShader->customDataCount);
				}

				// webgpu does not accept empty buffers
				if (uniArmatures.empty())
					uniArmatures.resize(1); // todo remove when possible
				if (uniCustomData.empty())
					uniCustomData.resize(4);

				DrawConfig draw;
				GraphicsBindingsCreateConfig bind;
				{
					const auto ab = aggregate->writeStruct(uniOptions, 0, true);
					bind.buffers.push_back(ab);
					draw.dynamicOffsets.push_back(ab);
				}
				{
					const auto ab = aggregate->writeArray<UniMesh>(uniMeshes, 1, false);
					bind.buffers.push_back(ab);
					draw.dynamicOffsets.push_back(ab);
				}
				{
					const auto ab = aggregate->writeArray<Mat3x4>(uniArmatures, 2, false);
					bind.buffers.push_back(ab);
					draw.dynamicOffsets.push_back(ab);
				}
				{
					const auto ab = aggregate->writeArray<float>(uniCustomData, 3, false);
					bind.buffers.push_back(ab);
					draw.dynamicOffsets.push_back(ab);
				}

				if (renderMode == RenderModeEnum::Color)
				{
					draw.depthTest = any(rm.mesh->renderFlags & MeshRenderFlags::DepthTest) ? DepthTestEnum::LessEqual : DepthTestEnum::Always;
					draw.depthWrite = any(rm.mesh->renderFlags & MeshRenderFlags::DepthWrite);
					draw.blending = rd.translucent ? BlendingEnum::PremultipliedTransparency : BlendingEnum::None;
				}
				else
				{
					draw.depthTest = any(rm.mesh->renderFlags & MeshRenderFlags::DepthTest) ? DepthTestEnum::Less : DepthTestEnum::Always;
					draw.depthWrite = true;
					draw.blending = BlendingEnum::None;
				}
				draw.backFaceCulling = none(rm.mesh->renderFlags & MeshRenderFlags::TwoSided);
				draw.model = +rm.mesh;
				draw.shader = +shader;
				draw.bindings = newGraphicsBindings(device, bind);
				draw.instances = instances.size();
				encoder->draw(draw);
			}

			CAGE_FORCE_INLINE GraphicsBindingsCreateConfig iconsMaterialBinding(Model *model, Texture *texture) const
			{
				GraphicsBindingsCreateConfig bind;
				if (model->materialBuffer && model->materialBuffer->size() > 0)
				{
					GraphicsBindingsCreateConfig::BufferBindingConfig bc;
					bc.buffer = +model->materialBuffer;
					bc.binding = 0;
					bc.uniform = true;
					bind.buffers.push_back(std::move(bc));
				}
				bind.textures.reserve(MaxTexturesCountPerMaterial);
				{
					GraphicsBindingsCreateConfig::TextureBindingConfig tc;
					tc.texture = texture;
					tc.binding = 1;
					bind.textures.push_back(std::move(tc));
				}
				Texture *dummy = privat::getTextureDummy2d(device);
				for (uint32 i = 1; i < MaxTexturesCountPerMaterial; i++)
				{
					GraphicsBindingsCreateConfig::TextureBindingConfig tc;
					tc.texture = dummy;
					tc.binding = i * 2 + 1;
					bind.textures.push_back(std::move(tc));
				}
				return bind;
			}

			void renderIcons(const RenderModeEnum renderMode, const PointerRange<const RenderData> instances) const
			{
				CAGE_ASSERT(!instances.empty());
				const RenderData &rd = instances[0];
				CAGE_ASSERT(std::holds_alternative<RenderIcon>(rd.data));

				if (renderMode != RenderModeEnum::Color)
					return;

				const Holder<Model> &mesh = std::get<RenderIcon>(rd.data).mesh;
				const Holder<Texture> &texture = std::get<RenderIcon>(rd.data).texture;

				prepareModelBindings(device, assets, +mesh); // todo remove

				Holder<MultiShader> multiShader = mesh->shaderName ? assets->get<MultiShader>(mesh->shaderName) : shaderIcon.share();
				Holder<Shader> shader = pickShaderVariant(+multiShader, +mesh, textureShaderVariant(texture->flags | (TextureFlags)(1u << 31)), renderMode, false);

				UniOptions uniOptions;

				uniMeshes.clear();
				uniMeshes.reserve(instances.size());
				CAGE_ASSERT(multiShader->customDataCount <= 16);
				uniCustomData.clear();
				if (multiShader->customDataCount)
					uniCustomData.reserve(multiShader->customDataCount * instances.size());

				for (const auto &inst : instances)
				{
					CAGE_ASSERT(std::holds_alternative<RenderIcon>(inst.data));
					CAGE_ASSERT(+std::get<RenderIcon>(inst.data).mesh == +mesh);
					CAGE_ASSERT(+std::get<RenderIcon>(inst.data).texture == +texture);
					uniMeshes.push_back(makeMeshUni(inst));
					appendShaderCustomData(inst.e, multiShader->customDataCount);
				}

				// webgpu does not accept empty buffers
				if (uniCustomData.empty())
					uniCustomData.resize(4);

				DrawConfig draw;
				GraphicsBindingsCreateConfig bind;
				{
					const auto ab = aggregate->writeStruct(uniOptions, 0, true);
					bind.buffers.push_back(ab);
					draw.dynamicOffsets.push_back(ab);
				}
				{
					const auto ab = aggregate->writeArray<UniMesh>(uniMeshes, 1, false);
					bind.buffers.push_back(ab);
					draw.dynamicOffsets.push_back(ab);
				}
				{
					bind.buffers.push_back({ privat::getBufferDummy(device), 2 });
				}
				{
					const auto ab = aggregate->writeArray<float>(uniCustomData, 3, false);
					bind.buffers.push_back(ab);
					draw.dynamicOffsets.push_back(ab);
				}

				draw.depthTest = any(mesh->renderFlags & MeshRenderFlags::DepthTest) ? DepthTestEnum::LessEqual : DepthTestEnum::Always;
				draw.depthWrite = any(mesh->renderFlags & MeshRenderFlags::DepthWrite);
				draw.blending = rd.translucent ? BlendingEnum::AlphaTransparency : BlendingEnum::None;
				draw.backFaceCulling = none(mesh->renderFlags & MeshRenderFlags::TwoSided);
				draw.model = +mesh;
				draw.shader = +shader;
				draw.materialOverride = newGraphicsBindings(device, iconsMaterialBinding(+mesh, +texture));
				draw.bindings = newGraphicsBindings(device, bind);
				draw.instances = instances.size();
				encoder->draw(draw);
			}

			void renderTexts(const RenderModeEnum renderMode, const PointerRange<const RenderData> instances) const
			{
				CAGE_ASSERT(!instances.empty());
				const RenderData &rd = instances[0];
				CAGE_ASSERT(std::holds_alternative<RenderText>(rd.data));

				if (renderMode != RenderModeEnum::Color)
					return;

				for (const auto &it : instances)
				{
					CAGE_ASSERT(std::holds_alternative<RenderText>(it.data));
					FontRenderConfig cfg;
					cfg.transform = viewProj * it.model;
					cfg.color = it.color;
					cfg.encoder = +encoder;
					cfg.aggregate = +aggregate;
					cfg.assets = onDemand;
					cfg.depthTest = true;
					cfg.guiShader = false;
					const RenderText &t = std::get<RenderText>(it.data);
					t.font->render(t.layout, cfg);
				}
			}

			void renderCustom(const RenderModeEnum renderMode, const PointerRange<const RenderData> instances) const
			{
				CAGE_ASSERT(!instances.empty());
				const RenderData &rd = instances[0];
				CAGE_ASSERT(std::holds_alternative<RenderCustom>(rd.data));

				if (renderMode != RenderModeEnum::Color)
					return;

				for (const auto &it : instances)
				{
					const ProfilingScope profiling("custom draw");
					CAGE_ASSERT(std::holds_alternative<RenderCustom>(it.data));
					CustomDrawConfig cfg;
					cfg.renderConfig = this;
					cfg.entity = it.e;
					cfg.encoder = +encoder;
					cfg.aggregate = +aggregate;
					CAGE_ASSERT(it.e->has<CustomDrawComponent>());
					it.e->value<CustomDrawComponent>().callback(cfg);
				}
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
						if constexpr (std::is_same_v<T, RenderCustom>)
							renderCustom(renderMode, instances);
					},
					rd.data);
			}

			void renderPass(const RenderModeEnum renderMode) const
			{
				switch (renderMode)
				{
					case RenderModeEnum::Shadowmap:
						CAGE_ASSERT(this->shadowmapData);
						CAGE_ASSERT(!this->cameraData);
						break;
					default:
						CAGE_ASSERT(!this->shadowmapData);
						CAGE_ASSERT(this->cameraData);
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
							if constexpr (std::is_same_v<T, RenderCustom>)
								return { d.e, nullptr, false, d.translucent };
							return {};
						},
						d.data);
				};
				const auto &render = [&](PointerRange<const RenderData> instances) { renderInstances(renderMode, instances); };
				partition(PointerRange<const RenderData>(renderData), mark, render);
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

				if (shadowmapData && none(rm.mesh->renderFlags & MeshRenderFlags::ShadowCast))
					return;

				const Mat4 mvpMat = viewProj * rd.model;
				if (!intersects(rm.mesh->boundingBox, Frustum(mvpMat)))
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

				if (!rm.mesh->bonesCount)
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
				rd.renderLayer = render.renderLayer + rm.mesh->renderLayer;
				rd.translucent = any(rm.mesh->renderFlags & (MeshRenderFlags::Transparent | MeshRenderFlags::Fade)) || rd.color[3] < 1;
				if (rd.translucent)
					rd.depth = (mvpMat * Vec4(0, 0, 0, 1))[2] * -1;

				if (rm.skeletalAnimation && cnfRenderSkeletonBones)
					prepareModelBones(rd);
				else
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

			void prepareCustomDraw(Entity *e, const CustomDrawComponent &cdc)
			{
				CAGE_ASSERT(cdc.callback);
				RenderCustom rc;
				RenderData rd;
				rd.e = e;
				rd.model = Mat4(modelTransform(e));
				rd.color = initializeColor(e->getOrDefault<ColorComponent>());
				rd.renderLayer = cdc.renderLayer;
				rd.translucent = true;
				rd.depth = (viewProj * (rd.model * Vec4(0, 0, 0, 1)))[2] * -1;
				rd.data = std::move(rc);
				renderData.push_back(std::move(rd));
			}

			CAGE_FORCE_INLINE bool failedMask(Entity *e) const
			{
				const uint32 c = e->getOrDefault<SceneComponent>().sceneMask;
				return (c & cameraSceneMask) == 0;
			}

			void prepareEntitiesModels()
			{
				ProfilingScope profiling("models");

				struct Data
				{
					Entity *e = nullptr;
					uint32 id = 0;
				};
				std::vector<Data> data;
				data.reserve(scene->component<ModelComponent>()->count());

				entitiesVisitor(
					[&](Entity *e, const ModelComponent &rc)
					{
						if (!rc.model)
							return;
						if (failedMask(e))
							return;
						data.push_back({ e, rc.model });
					},
					+scene, false);
				profiling.set(Stringizer() + "models: " + data.size());
				std::sort(data.begin(), data.end(), [](const Data &a, const Data &b) { return a.id < b.id; });

				const auto &mark = [](const Data &data) { return data.id; };
				const auto &output = [&](PointerRange<const Data> data)
				{
					CAGE_ASSERT(data.size() > 0);

					if (Holder<RenderObject> obj = assets->get<AssetSchemeIndexRenderObject, RenderObject>(data[0].id))
					{
						CAGE_ASSERT(obj->lodsCount() > 0);
						if (obj->lodsCount() == 1)
						{
							prepareModels.clear();
							for (uint32 id : obj->models(0))
							{
								if (Holder<Model> mesh = onDemand->get<AssetSchemeIndexModel, Model>(id))
									prepareModels.push_back(std::move(mesh));
							}
							if (prepareModels.empty())
								return;
							for (const auto &it : data)
							{
								const Mat4 mm = Mat4(modelTransform(it.e));
								for (const auto &mesh : prepareModels)
								{
									RenderModel rm;
									rm.mesh = mesh.share();
									RenderData rd;
									rd.e = it.e;
									rd.model = mm;
									rd.data = std::move(rm);
									prepareModel(rd);
								}
							}
							return;
						}

						for (const auto &it : data)
						{
							RenderData rd;
							rd.e = it.e;
							rd.model = Mat4(modelTransform(it.e));
							lodSelection.selectModels(prepareModels, Vec3(rd.model * Vec4(0, 0, 0, 1)), +obj, +onDemand);
							for (auto &it : prepareModels)
							{
								RenderData d;
								d.model = rd.model;
								d.e = rd.e;
								RenderModel r;
								r.mesh = std::move(it);
								d.data = std::move(r);
								prepareModel(d, +obj);
							}
						}
						return;
					}

					if (Holder<Model> mesh = assets->get<AssetSchemeIndexModel, Model>(data[0].id))
					{
						for (const auto &it : data)
						{
							RenderModel rm;
							rm.mesh = mesh.share();
							RenderData rd;
							rd.e = it.e;
							rd.model = Mat4(modelTransform(it.e));
							rd.data = std::move(rm);
							prepareModel(rd);
						}
						return;
					}

					if (cnfRenderMissingModels)
					{
						Holder<Model> mesh = assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/fake.obj"));
						if (!mesh)
							return;
						for (const auto &it : data)
						{
							RenderModel rm;
							rm.mesh = mesh.share();
							RenderData rd;
							rd.e = it.e;
							rd.model = Mat4(modelTransform(it.e));
							rd.data = std::move(rm);
							prepareModel(rd);
						}
					}
				};
				partition(PointerRange<const Data>(data), mark, output);
			}

			void prepareEntitiesIcons()
			{
				ProfilingScope profiling("icons");

				struct Data
				{
					IconComponent ic;
					Entity *e = nullptr;
				};
				std::vector<Data> data;
				data.reserve(scene->component<IconComponent>()->count());

				entitiesVisitor(
					[&](Entity *e, const IconComponent &ic)
					{
						if (!ic.icon)
							return;
						if (failedMask(e))
							return;
						data.push_back({ ic, e });
					},
					+scene, false);
				profiling.set(Stringizer() + "icons: " + data.size());
				std::sort(data.begin(), data.end(), [](const Data &a, const Data &b) { return std::pair{ a.ic.icon, a.ic.model } < std::pair{ b.ic.icon, b.ic.model }; });

				const auto &mark = [](const Data &data) { return std::pair{ data.ic.icon, data.ic.model }; };
				const auto &output = [&](PointerRange<const Data> data)
				{
					CAGE_ASSERT(data.size() > 0);
					Holder<Texture> tex = assets->get<AssetSchemeIndexTexture, Texture>(data[0].ic.icon);
					if (!tex)
						return;
					Holder<Model> mod = data[0].ic.model ? assets->get<AssetSchemeIndexModel, Model>(data[0].ic.model) : modelIcon.share();
					if (!mod)
						return;
					for (const auto &it : data)
					{
						const IconComponent &ic = it.e->value<IconComponent>();
						RenderIcon ri;
						ri.texture = tex.share();
						ri.mesh = mod.share();
						CAGE_ASSERT(ri.mesh->bonesCount == 0);
						RenderData rd;
						rd.e = it.e;
						rd.model = Mat4(modelTransform(it.e));
						rd.color = initializeColor(it.e->getOrDefault<ColorComponent>());
						const uint64 startTime = rd.e->getOrDefault<SpawnTimeComponent>().spawnTime;
						rd.animation = Vec4((double)(sint64)(currentTime - startTime) / (double)1'000'000, 1, 0, 0);
						rd.renderLayer = ic.renderLayer + ri.mesh->renderLayer;
						rd.translucent = true;
						rd.depth = (viewProj * (rd.model * Vec4(0, 0, 0, 1)))[2] * -1;
						rd.data = std::move(ri);
						renderData.push_back(std::move(rd));
					}
				};
				partition(PointerRange<const Data>(data), mark, output);
			}

			void prepareEntities()
			{
				ProfilingScope profiling("prepare entities");
				profiling.set(Stringizer() + "entities: " + scene->count());

				renderData.reserve(scene->component<ModelComponent>()->count() + scene->component<IconComponent>()->count() + scene->component<TextComponent>()->count());

				prepareEntitiesModels();
				prepareEntitiesIcons();

				entitiesVisitor(
					[&](Entity *e, const TextComponent &tc)
					{
						if (failedMask(e))
							return;
						prepareText(e, tc);
					},
					+scene, false);

				entitiesVisitor(
					[&](Entity *e, const CustomDrawComponent &cdc)
					{
						if (failedMask(e))
							return;
						prepareCustomDraw(e, cdc);
					},
					+scene, false);
			}

			void orderRenderData()
			{
				ProfilingScope profiling("order render data");
				profiling.set(Stringizer() + "count: " + renderData.size());
				std::sort(renderData.begin(), renderData.end(), [](const RenderData &a, const RenderData &b) { return a.cmp() < b.cmp(); });
			}

			void prepareCameraLights()
			{
				ProfilingScope profiling("prepare lights");

				CAGE_ASSERT(cameraData);

				{ // add unshadowed lights
					std::vector<UniLight> lights;
					lights.reserve(100);
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
					cameraData->lightsCount = numeric_cast<uint32>(lights.size());
					if (lights.empty())
						lights.resize(1); // must ensure non-empty buffer
					buffLights = newGraphicsBuffer(device, lights.size() * sizeof(UniLight), "UniLight");
					buffLights->writeArray<UniLight>(lights);
				}

				{ // add shadowed lights
					std::vector<UniShadowedLight> shadows;
					uint32 tex2dCount = 0, texCubeCount = 0;
					shadows.reserve(10);
					for (auto &sh_ : cameraData->shadowmaps)
					{
						CAGE_ASSERT(sh_->shadowmapData);
						ShadowmapData &sh = *sh_->shadowmapData;
						if (sh.cascade != 0)
							continue;
						if (sh.lightComponent.lightType == LightTypeEnum::Point)
						{
							if (texCubeCount >= 8)
								CAGE_THROW_ERROR(Exception, "too many shadowmaps (cube shadows)");
							sh.shadowUni.shadowParams[0] = texCubeCount;
							texCubeCount++;
						}
						else
						{
							if (tex2dCount >= 8)
								CAGE_THROW_ERROR(Exception, "too many shadowmaps (2D shadows)");
							sh.shadowUni.shadowParams[0] = tex2dCount;
							tex2dCount++;
						}
						shadows.push_back(sh.shadowUni);
					}
					CAGE_ASSERT(shadows.size() == tex2dCount + texCubeCount);
					cameraData->shadowedLightsCount = numeric_cast<uint32>(shadows.size());
					if (shadows.empty())
						shadows.resize(1); // must ensure non-empty buffer
					buffShadowedLights = newGraphicsBuffer(device, shadows.size() * sizeof(UniShadowedLight), "UniShadowedLight");
					buffShadowedLights->writeArray<UniShadowedLight>(shadows);
				}

				profiling.set(Stringizer() + "count: " + cameraData->lightsCount + ", shadowed: " + cameraData->shadowedLightsCount);
			}

			Holder<Texture> createDepthTextureTarget()
			{
				TransientTextureCreateConfig conf;
				conf.name = "depth target";
				conf.resolution = Vec3i(resolution, 1);
				conf.format = wgpu::TextureFormat::Depth32Float;
				return newTexture(device, conf);
			}

			Holder<Texture> createDepthSamplingTexture()
			{
				TransientTextureCreateConfig conf;
				conf.name = "depth sampling";
				conf.resolution = Vec3i(resolution, 1);
				conf.format = wgpu::TextureFormat::R32Float;
				return newTexture(device, conf);
			}

			Holder<Texture> createColorTextureTarget(const AssetLabel &name)
			{
				TransientTextureCreateConfig conf;
				conf.name = name;
				conf.resolution = Vec3i(resolution, 1);
				conf.format = wgpu::TextureFormat::RGBA16Float;
				return newTexture(device, conf);
			}

			Holder<Texture> createSsaoTextureTarget()
			{
				static constexpr int downscale = 3;
				TransientTextureCreateConfig conf;
				conf.name = "ssao target";
				conf.resolution = Vec3i(resolution, 0) / downscale;
				conf.resolution[2] = 1;
				conf.format = wgpu::TextureFormat::R16Float;
				return newTexture(device, conf);
			}

			void taskCamera(uint32)
			{
				CAGE_ASSERT(cameraData);
				CAGE_ASSERT(!shadowmapData);

				encoder = newGraphicsEncoder(device, "scene render camera");
				aggregate = newGraphicsAggregateBuffer({ device });
				frustum = Frustum(viewProj);
				prepareEntities();
				orderRenderData();
				prepareCameraLights();

				Holder<Texture> depthTexture = createDepthTextureTarget();
				Holder<Texture> depthSampling = createDepthSamplingTexture();
				Holder<Texture> colorTexture = createColorTextureTarget("color target");
				Holder<Texture> ssaoTexture = createSsaoTextureTarget();

				GraphicsBindings globalBindings;
				{
					GraphicsBindingsCreateConfig bind;
					bind.buffers.push_back({ .buffer = +buffViewport, .binding = 0, .uniform = true });
					bind.buffers.push_back({ .buffer = +buffProjection, .binding = 1, .uniform = true });
					bind.buffers.push_back({ +buffLights, 2 });
					bind.buffers.push_back({ +buffShadowedLights, 3 });
					bind.textures.push_back({ +ssaoTexture, 4 });
					bind.textures.push_back({ +depthSampling, 6 });

					std::array<Texture *, 8> sh2d = {}, shCube = {};
					for (const auto &it : cameraData->shadowmaps)
					{
						CAGE_ASSERT(it->shadowmapData);
						const ShadowmapData &shd = *it->shadowmapData;
						if (shd.cascade != 0)
							continue;
						const uint32 index = numeric_cast<uint32>(shd.shadowUni.shadowParams[0]);
						CAGE_ASSERT(index < 8);
						switch (shd.lightComponent.lightType)
						{
							case LightTypeEnum::Directional:
							case LightTypeEnum::Spot:
								CAGE_ASSERT(!sh2d[index]);
								sh2d[index] = +shd.shadowTexture;
								break;
							case LightTypeEnum::Point:
								CAGE_ASSERT(!shCube[index]);
								shCube[index] = +shd.shadowTexture;
								break;
						}
					}
					for (uint32 i = 0; i < 8; i++)
					{
						if (!sh2d[i])
							sh2d[i] = privat::getTextureDummyArray(device);
						if (!shCube[i])
							shCube[i] = privat::getTextureDummyCube(device);
					}

					for (uint32 i = 0; i < 8; i++)
					{
						bind.textures.push_back(GraphicsBindingsCreateConfig::TextureBindingConfig{ .texture = sh2d[i], .binding = 16 + i, .bindSampler = false });
						bind.textures.push_back(GraphicsBindingsCreateConfig::TextureBindingConfig{ .texture = shCube[i], .binding = 24 + i, .bindSampler = false });
					}

					// shadowmap sampler
					bind.textures.push_back(GraphicsBindingsCreateConfig::TextureBindingConfig{ .texture = privat::getTextureShadowsSampler(device), .binding = 15, .bindTexture = false });

					globalBindings = newGraphicsBindings(device, bind);
				}

				{ // depth prepass
					const ProfilingScope profiling("depth prepass");
					RenderPassConfig passcfg;
					passcfg.depthTarget = { +depthTexture, true };
					passcfg.bindings = globalBindings;
					encoder->nextPass(passcfg);
					const auto scope = encoder->namedScope("depth prepass");
					renderPass(RenderModeEnum::DepthPrepass);
				}

				{ // copy depth
					RenderPassConfig passcfg;
					passcfg.colorTargets.push_back({ +depthSampling });
					encoder->nextPass(passcfg);
					const auto scope = encoder->namedScope("copy depth");
					DrawConfig drawcfg;
					prepareModelBindings(device, assets, +modelSquare); // todo remove
					drawcfg.model = +modelSquare;
					drawcfg.shader = +shaderBlitPixels;
					GraphicsBindingsCreateConfig bind;
					bind.textures.push_back({ +depthTexture, 0 });
					drawcfg.bindings = newGraphicsBindings(device, bind);
					encoder->draw(drawcfg);
				}

				ScreenSpaceCommonConfig commonConfig; // helper to simplify initialization
				commonConfig.assets = assets;
				commonConfig.encoder = +encoder;
				commonConfig.aggregate = +aggregate;
				commonConfig.resolution = resolution;

				// ssao
				if (any(effects.effects & ScreenSpaceEffectsFlags::AmbientOcclusion))
				{
					ScreenSpaceAmbientOcclusionConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					(ScreenSpaceAmbientOcclusion &)cfg = effects.ssao;
					cfg.proj = projection;
					cfg.inDepth = +depthSampling;
					cfg.outAo = +ssaoTexture;
					cfg.frameIndex = frameIndex;
					screenSpaceAmbientOcclusion(cfg);
				}

				{ // color pass
					const ProfilingScope profiling("color pass");
					RenderPassConfig passcfg;
					passcfg.colorTargets.push_back({ +colorTexture });
					passcfg.depthTarget = { +depthTexture, false };
					passcfg.bindings = globalBindings;
					encoder->nextPass(passcfg);
					const auto scope = encoder->namedScope("color pass");
					renderPass(RenderModeEnum::Color);
				}

				{
					const ProfilingScope profiling("effects");

					Holder<Texture> texSource = colorTexture.share();
					Holder<Texture> texTarget = createColorTextureTarget("effects target");

					// depth of field
					if (any(effects.effects & ScreenSpaceEffectsFlags::DepthOfField))
					{
						ScreenSpaceDepthOfFieldConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						(ScreenSpaceDepthOfField &)cfg = effects.depthOfField;
						cfg.proj = projection;
						cfg.inDepth = +depthTexture;
						cfg.inColor = +texSource;
						cfg.outColor = +texTarget;
						screenSpaceDepthOfField(cfg);
						std::swap(texSource, texTarget);
					}

					// bloom
					if (any(effects.effects & ScreenSpaceEffectsFlags::Bloom))
					{
						ScreenSpaceBloomConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						(ScreenSpaceBloom &)cfg = effects.bloom;
						cfg.inColor = +texSource;
						cfg.outColor = +texTarget;
						screenSpaceBloom(cfg);
						std::swap(texSource, texTarget);
					}

					// tonemap & gamma correction
					if (any(effects.effects & (ScreenSpaceEffectsFlags::ToneMapping | ScreenSpaceEffectsFlags::GammaCorrection)))
					{
						ScreenSpaceTonemapConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						cfg.inColor = +texSource;
						cfg.outColor = +texTarget;
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
						cfg.inColor = +texSource;
						cfg.outColor = +texTarget;
						screenSpaceFastApproximateAntiAliasing(cfg);
						std::swap(texSource, texTarget);
					}

					// sharpening
					if (any(effects.effects & ScreenSpaceEffectsFlags::Sharpening) && effects.sharpening.strength > 1e-3)
					{
						ScreenSpaceSharpeningConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						cfg.inColor = +texSource;
						cfg.outColor = +texTarget;
						cfg.strength = effects.sharpening.strength;
						screenSpaceSharpening(cfg);
						std::swap(texSource, texTarget);
					}

					colorTexture = std::move(texSource);
				}

				// final blit to target

				{
					const ProfilingScope profiling("final blit");
					RenderPassConfig passcfg;
					passcfg.colorTargets.push_back({ target });
					encoder->nextPass(passcfg);
					const auto scope = encoder->namedScope("final blit");
					DrawConfig drawcfg;
					prepareModelBindings(device, assets, +modelSquare); // todo remove
					drawcfg.model = +modelSquare;
					if (resolution == target->resolution())
						drawcfg.shader = +shaderBlitPixels;
					else
						drawcfg.shader = +shaderBlitScaled;
					GraphicsBindingsCreateConfig bind;
					bind.textures.push_back({ +colorTexture, 0 });
					drawcfg.bindings = newGraphicsBindings(device, bind);
					encoder->draw(drawcfg);
				}

				aggregate->submit();
				aggregate.clear();
			}

			PointerRangeHolder<Holder<GraphicsEncoder>> prepareCamera()
			{
				PointerRangeHolder<Holder<GraphicsEncoder>> result;

				cameraData = CameraData();

				model = Mat4(transform);
				view = inverse(model);
				viewProj = projection * view;

				{
					UniViewport viewport;
					viewport.viewMat = view;
					viewport.eyePos = model * Vec4(0, 0, 0, 1);
					viewport.eyeDir = model * Vec4(0, 0, -1, 0);
					viewport.ambientLight = Vec4(colorGammaToLinear(camera.ambientColor) * camera.ambientIntensity, 0);
					viewport.skyLight = Vec4(colorGammaToLinear(camera.skyColor) * camera.skyIntensity, 0);
					viewport.viewport = Vec4(Vec2(), Vec2(resolution));
					viewport.time = Vec4(frameIndex % 10'000, (currentTime % uint64(1e6)) / 1e6, (currentTime % uint64(1e9)) / 1e9, 0);
					buffViewport = newGraphicsBuffer(+device, sizeof(viewport), "UniViewport");
					buffViewport->writeStruct(viewport);
				}

				{
					UniProjection uni;
					uni.viewMat = view;
					uni.projMat = projection;
					uni.vpMat = viewProj;
					uni.vpInv = inverse(viewProj);
					buffProjection = newGraphicsBuffer(+device, sizeof(uni), "UniProjection");
					buffProjection->writeStruct(uni);
				}

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
					//tasks.push_back(tasksRunAsync("render camera task", Delegate<void(uint32)>().bind<SceneRenderImpl, &SceneRenderImpl::taskCamera>(this)));
					{
						const ProfilingScope profiling("execute camera");
						taskCamera(0);
					}
					{
						const ProfilingScope profiling("waiting for shadowmaps");
						for (const auto &it : tasks)
							it->wait();
					}
				}

				for (const auto &it : cameraData->shadowmaps)
					result.push_back(std::move(it->encoder));
				result.push_back(std::move(encoder));
				return result;
			}

			Holder<Texture> createShadowmapCascadeView(Texture *tex, uint32 cascade)
			{
				wgpu::TextureViewDescriptor desc = {};
				desc.baseArrayLayer = cascade;
				desc.arrayLayerCount = 1;
				desc.label = "shadowmap cascade view";
				wgpu::TextureView view = tex->nativeTexture().CreateView(&desc);
				return newTexture(tex->nativeTexture(), view, nullptr, "shadowmap cascade view");
			}

			void taskShadowmap()
			{
				CAGE_ASSERT(!cameraData);
				CAGE_ASSERT(shadowmapData);

				encoder = newGraphicsEncoder(device, "scene render shadowmap");
				aggregate = newGraphicsAggregateBuffer({ device });

				{
					UniProjection uni;
					uni.viewMat = view;
					uni.projMat = projection;
					uni.vpMat = viewProj;
					uni.vpInv = inverse(viewProj);
					buffProjection = newGraphicsBuffer(+device, sizeof(uni), "UniProjection");
					buffProjection->writeStruct(uni);
				}

				GraphicsBindings globalBindings;
				{
					GraphicsBuffer *dummyStorage = privat::getBufferDummy(device);
					Texture *dummyRegular = privat::getTextureDummy2d(device);
					GraphicsBindingsCreateConfig bind;
					bind.buffers.push_back({ .buffer = +buffViewport, .binding = 0, .uniform = true });
					bind.buffers.push_back({ .buffer = +buffProjection, .binding = 1, .uniform = true });
					bind.buffers.push_back({ dummyStorage, 2 });
					bind.buffers.push_back({ dummyStorage, 3 });
					bind.textures.push_back({ dummyRegular, 4 });
					bind.textures.push_back({ dummyRegular, 6 });
					globalBindings = newGraphicsBindings(device, bind);
				}

				prepareEntities();
				orderRenderData();

				Holder<Texture> target = createShadowmapCascadeView(+shadowmapData->shadowTexture, shadowmapData->cascade);

				{
					const ProfilingScope profiling("shadowmap pass");
					RenderPassConfig passcfg;
					passcfg.depthTarget = { +target, true };
					passcfg.bindings = globalBindings;
					encoder->nextPass(passcfg);
					const auto scope = encoder->namedScope("shadowmap pass");
					renderPass(RenderModeEnum::Shadowmap);
				}

				aggregate->submit();
				aggregate.clear();
			}

			Holder<SceneRenderImpl> createShadowmapPipeline(Entity *e, const LightComponent &lc, const ShadowmapComponent &sc)
			{
				CAGE_ASSERT(e->id() != 0); // lights with shadowmap may not be anonymous

				Holder<SceneRenderImpl> data = systemMemory().createHolder<SceneRenderImpl>(this);
				this->cameraData->shadowmaps.push_back(data.share());

				data->shadowmapData = ShadowmapData();
				ShadowmapData &shadowmap = *data->shadowmapData;
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
				CAGE_ASSERT(shadowmapData->lightComponent.lightType == LightTypeEnum::Directional);
				CAGE_ASSERT(camera.shadowmapFrustumDepthFraction > 0 && camera.shadowmapFrustumDepthFraction <= 1);

				const auto &sc = shadowmapData->shadowmapComponent;
				CAGE_ASSERT(sc.cascadesSplitLogFactor >= 0 && sc.cascadesSplitLogFactor <= 1);
				CAGE_ASSERT(sc.cascadesCount > 0 && sc.cascadesCount <= 4);
				CAGE_ASSERT(sc.cascadesPaddingDistance >= 0);

				const Mat4 invP = inverse(projection);
				const auto &getPlane = [&](Real ndcZ) -> Real
				{
					const Vec4 a = invP * Vec4(0, 0, ndcZ, 1);
					return -a[2] / a[3];
				};
				const Real cameraNear = getPlane(0);
				const Real cameraFar = getPlane(1) * camera.shadowmapFrustumDepthFraction;
				CAGE_ASSERT(cameraNear > 0 && cameraFar > cameraNear);

				const auto &getSplit = [&](Real f) -> Real
				{
					const Real uniform = cameraNear + (cameraFar - cameraNear) * f;
					const Real log = cameraNear * pow(cameraFar / cameraNear, f);
					return interpolate(uniform, log, sc.cascadesSplitLogFactor);
				};
				const Real splitNear = getSplit(Real(shadowmapData->cascade + 0) / sc.cascadesCount);
				const Real splitFar = getSplit(Real(shadowmapData->cascade + 1) / sc.cascadesCount);
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
				static constexpr Mat4 bias = Mat4(0.5, 0, 0, 0, 0, -0.5, 0, 0, 0, 0, 1, 0, 0.5, 0.5, 0, 1);
				shadowmapData->shadowUni.shadowMat[shadowmapData->cascade] = bias * viewProj;
				shadowmapData->shadowUni.cascadesDepths[shadowmapData->cascade] = splitFar;
			}

			void initializeShadowmapSingle()
			{
				CAGE_ASSERT(shadowmapData->cascade == 0);

				view = inverse(model);
				projection = [&]()
				{
					const auto lc = shadowmapData->lightComponent;
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
				static constexpr Mat4 bias = Mat4(0.5, 0, 0, 0, 0, -0.5, 0, 0, 0, 0, 1, 0, 0.5, 0.5, 0, 1);
				shadowmapData->shadowUni.shadowMat[shadowmapData->cascade] = bias * viewProj;
			}

			Holder<Texture> createShadowmap2D(uint32 entityId, uint32 resolution, uint32 cascades)
			{
				TransientTextureCreateConfig conf;
				conf.name = "shadowmap target";
				conf.resolution = Vec3i(resolution, resolution, cascades);
				conf.format = wgpu::TextureFormat::Depth32Float;
				conf.flags = TextureFlags::Array;
				conf.entityId = entityId;
				return newTexture(device, conf);
			}

			Holder<Texture> createShadowmapCube(uint32 entityId, uint32 resolution)
			{
				TransientTextureCreateConfig conf;
				conf.name = "shadowmap target";
				conf.resolution = Vec3i(resolution, resolution, 6);
				conf.format = wgpu::TextureFormat::Depth16Unorm;
				conf.flags = TextureFlags::Cubemap;
				conf.entityId = entityId;
				return newTexture(device, conf);
			}

			void prepareShadowmap(std::vector<Holder<AsyncTask>> &outputTasks, Entity *e, const LightComponent &lc, const ShadowmapComponent &sc)
			{
				switch (lc.lightType)
				{
					case LightTypeEnum::Directional:
					{
						CAGE_ASSERT(sc.cascadesCount > 0 && sc.cascadesCount <= 4);
						Holder<Texture> tex = createShadowmap2D(e->id(), sc.resolution, sc.cascadesCount);
						ShadowmapData *first = nullptr;
						for (uint32 cascade = 0; cascade < sc.cascadesCount; cascade++)
						{
							Holder<SceneRenderImpl> data = createShadowmapPipeline(e, lc, sc);
							data->shadowmapData->cascade = cascade;
							data->shadowmapData->shadowTexture = tex.share();
							data->initializeShadowmapCascade();
							if (cascade == 0)
								first = &*data->shadowmapData;
							else
							{
								first->shadowUni.cascadesDepths[cascade] = data->shadowmapData->shadowUni.cascadesDepths[cascade];
								first->shadowUni.shadowMat[cascade] = data->shadowmapData->shadowUni.shadowMat[cascade];
							}
							outputTasks.push_back(tasksRunAsync<SceneRenderImpl>("render shadowmap cascade", [](SceneRenderImpl &impl, uint32) { impl.taskShadowmap(); }, std::move(data)));
						}
						break;
					}
					case LightTypeEnum::Spot:
					case LightTypeEnum::Point:
					{
						Holder<SceneRenderImpl> data = createShadowmapPipeline(e, lc, sc);
						data->initializeShadowmapSingle();
						if (lc.lightType == LightTypeEnum::Point)
							data->shadowmapData->shadowTexture = createShadowmapCube(e->id(), sc.resolution);
						else
							data->shadowmapData->shadowTexture = createShadowmap2D(e->id(), sc.resolution, 1);
						outputTasks.push_back(tasksRunAsync<SceneRenderImpl>("render shadowmap single", [](SceneRenderImpl &impl, uint32) { impl.taskShadowmap(); }, std::move(data)));
						break;
					}
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid light type");
				}
			}
		};
	}

	Holder<PointerRange<Holder<GraphicsEncoder>>> sceneRender(const SceneRenderConfig &config)
	{
		CAGE_ASSERT(config.device);
		CAGE_ASSERT(config.assets);
		CAGE_ASSERT(config.onDemand);
		CAGE_ASSERT(config.scene);
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

		SceneRenderImpl impl(config);
		if (impl.skeletonPreparatorCollection)
			return impl.prepareCamera();
		return {};
	}
}
