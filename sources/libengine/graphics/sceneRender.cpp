#include <algorithm>
#include <array>
#include <atomic>
#include <cmath> // std::floor
#include <map>
#include <vector>

#include <unordered_dense.h>

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
#include <cage-core/stdHash.h>
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

		struct SceneModel : private Noncopyable
		{
			Holder<SkeletalAnimationPreparatorInstance> skeletalAnimation;
			Model *mesh = nullptr;
		};

		struct SceneSprite : private Noncopyable
		{
			Texture *texture = nullptr;
			Model *mesh = nullptr;
		};

		struct SceneText : private Noncopyable
		{
			Holder<FontLayoutResult> layout;
			Font *font = nullptr;
		};

		struct SceneCustom : private Noncopyable
		{};

		enum class VariantEnum : uint8
		{
			None,
			Model,
			Sprite,
			Text,
			Custom,
		};

		struct SceneItemVariant : private Immovable
		{
			union Variants
			{
				SceneModel model;
				SceneSprite sprite;
				SceneText text;
				SceneCustom custom;
				Variants() {}
				~Variants() {}
			};

			Variants variants;
			VariantEnum index = VariantEnum::None;

			CAGE_FORCE_INLINE SceneModel &model()
			{
				CAGE_ASSERT(index == VariantEnum::Model);
				return variants.model;
			}

			CAGE_FORCE_INLINE const SceneModel &model() const
			{
				CAGE_ASSERT(index == VariantEnum::Model);
				return variants.model;
			}

			CAGE_FORCE_INLINE SceneSprite &sprite()
			{
				CAGE_ASSERT(index == VariantEnum::Sprite);
				return variants.sprite;
			}

			CAGE_FORCE_INLINE const SceneSprite &sprite() const
			{
				CAGE_ASSERT(index == VariantEnum::Sprite);
				return variants.sprite;
			}

			CAGE_FORCE_INLINE SceneText &text()
			{
				CAGE_ASSERT(index == VariantEnum::Text);
				return variants.text;
			}

			CAGE_FORCE_INLINE const SceneText &text() const
			{
				CAGE_ASSERT(index == VariantEnum::Text);
				return variants.text;
			}

			CAGE_FORCE_INLINE SceneCustom &custom()
			{
				CAGE_ASSERT(index == VariantEnum::Custom);
				return variants.custom;
			}

			CAGE_FORCE_INLINE const SceneCustom &custom() const
			{
				CAGE_ASSERT(index == VariantEnum::Custom);
				return variants.custom;
			}

			CAGE_FORCE_INLINE SceneItemVariant() {}

			CAGE_FORCE_INLINE ~SceneItemVariant() { destroy(); }

			CAGE_FORCE_INLINE void assign(SceneModel &&v)
			{
				destroy();
				new (&variants.model) SceneModel(std::move(v));
				index = VariantEnum::Model;
			}

			CAGE_FORCE_INLINE void assign(SceneSprite &&v)
			{
				destroy();
				new (&variants.sprite) SceneSprite(std::move(v));
				index = VariantEnum::Sprite;
			}

			CAGE_FORCE_INLINE void assign(SceneText &&v)
			{
				destroy();
				new (&variants.text) SceneText(std::move(v));
				index = VariantEnum::Text;
			}

			CAGE_FORCE_INLINE void assign(SceneCustom &&v)
			{
				destroy();
				new (&variants.custom) SceneCustom(std::move(v));
				index = VariantEnum::Custom;
			}

			CAGE_FORCE_INLINE void destroy() noexcept
			{
				switch (index)
				{
					case VariantEnum::None:
						break;
					case VariantEnum::Model:
						variants.model.~SceneModel();
						break;
					case VariantEnum::Sprite:
						variants.sprite.~SceneSprite();
						break;
					case VariantEnum::Text:
						variants.text.~SceneText();
						break;
					case VariantEnum::Custom:
						variants.custom.~SceneCustom();
						break;
				}
				index = VariantEnum::None;
			}
		};

		struct SceneItem : private Noncopyable
		{
			SceneItemVariant data;
			Transform transform;
			Vec4 color = Vec4::Nan(); // linear rgb (NOT alpha-premultiplied), opacity
			Vec4 animation = Vec4::Nan(); // time (seconds), speed, offset (normalized), unused
			Entity *e = nullptr;
			sint32 renderLayer = 0;
			bool shadowCast = false;
			bool orderDependent = false;
			bool blending = false;

			SceneItem() = default;

			SceneItem(SceneItem &&other) noexcept
			{
				detail::memcpy(this, &other, sizeof(SceneItem)); // performance hack
				other.data.index = VariantEnum::None;
			}

			SceneItem &operator=(SceneItem &&other) noexcept
			{
				if (&other == this)
					return *this;
				this->~SceneItem();
				detail::memcpy(this, &other, sizeof(SceneItem)); // performance hack
				other.data.index = VariantEnum::None;
				return *this;
			}
		};

		struct RenderItem
		{
			const SceneItem *base = nullptr;
			Real depth;

			const SceneItem *operator->() const { return base; }

			// exact order for correct rendering
			std::tuple<sint32, bool, Real, void *, void *, bool> cmp() const noexcept
			{
				switch (base->data.index)
				{
					case VariantEnum::None:
						return {};
					case VariantEnum::Model:
						return { base->renderLayer, base->orderDependent, depth, base->data.model().mesh, nullptr, !!base->data.model().skeletalAnimation };
					case VariantEnum::Sprite:
						return { base->renderLayer, base->orderDependent, depth, base->data.sprite().mesh, base->data.sprite().texture, false };
					case VariantEnum::Text:
						return { base->renderLayer, base->orderDependent, depth, base->data.text().font, nullptr, false };
					case VariantEnum::Custom:
						return { base->renderLayer, base->orderDependent, depth, nullptr, nullptr, false };
				}
				return {};
			}

			// used for partitioning render items for instancing
			std::tuple<void *, void *, bool, bool> mark() const noexcept
			{
				const SceneItem &d = *base;
				switch (d.data.index)
				{
					case VariantEnum::None:
						return {};
					case VariantEnum::Model:
						return { d.data.model().mesh, nullptr, !!d.data.model().skeletalAnimation, d.orderDependent };
					case VariantEnum::Sprite:
						return { d.data.sprite().mesh, d.data.sprite().texture, false, d.orderDependent };
					case VariantEnum::Text:
						return { d.data.text().font, nullptr, false, d.orderDependent };
					case VariantEnum::Custom:
						return { d.e, nullptr, false, d.orderDependent };
				}
				return {};
			};
		};

		enum class RenderModeEnum
		{
			Shadowmap,
			DepthPrepass,
			Color,
		};

		std::atomic<uintPtr> ItemsContainerReservation = 0;

		// container with stable pointers to items
		// never realocates
		// voids inserted items when full
		template<class T>
		struct ItemsContainer : private std::vector<T>, Immovable
		{
			ItemsContainer() { reserve(ItemsContainerReservation); }

			~ItemsContainer() { ItemsContainerReservation = std::max((uintPtr)ItemsContainerReservation, (uintPtr)capacity()); }

			using typename std::vector<T>::value_type;
			using std::vector<T>::operator[];
			using std::vector<T>::begin;
			using std::vector<T>::end;
			using std::vector<T>::data;
			using std::vector<T>::size;
			using std::vector<T>::capacity;

			template<class U>
			CAGE_FORCE_INLINE T *push_back(U &&v)
			{
				if (size() == capacity())
				{
					ItemsContainerReservation++;
					return nullptr;
				}
				std::vector<T>::push_back(std::forward<U>(v));
				return &std::vector<T>::back();
			}

			CAGE_FORCE_INLINE void reserve(uintPtr v)
			{
				if (std::vector<T>::empty() && v > capacity())
					std::vector<T>::reserve(v);
			}

			template<class Tst>
			CAGE_FORCE_INLINE void erase_if(Tst tst)
			{
				std::erase_if(*this, tst);
			}
		};

		template<class T, class Mark, class Output>
		void partition(PointerRange<T> inputRange, Mark &&mark, Output &&output)
		{
			ProfilingScope profiling("partition");
			uint32 parts = 0;
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
				parts++;
			}
			profiling.set(Stringizer() + parts + " / " + inputRange.size());
		}

		CAGE_FORCE_INLINE Vec4 initializeColor(const ColorComponent &cc)
		{
			const Vec3 c = valid(cc.color) ? colorGammaToLinear(cc.color) : Vec3(1);
			const Real i = valid(cc.intensity) ? cc.intensity : 1;
			const Real o = valid(cc.opacity) ? cc.opacity : 1;
			return Vec4(c * i, o);
		}

		UniLight initializeLightUni(const Mat4 &model, const LightComponent &lc, const ColorComponent &cc, const Vec3 &cameraCenter)
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

		void filterLightsOverLimit(std::vector<UniLight> &lights, uint32 limit)
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

		CAGE_FORCE_INLINE uint32 textureShaderVariant(const TextureFlags flags)
		{
			if (flags == TextureFlags::None)
				return 0;
			if (any(flags & TextureFlags::Cubemap))
				return HashString("MaterialTexCube");
			if (any(flags & TextureFlags::Array))
				return HashString("MaterialTexArray");
			return HashString("MaterialTexRegular");
		}

		CAGE_FORCE_INLINE GraphicsBindingsCreateConfig spritesMaterialBinding(Model *model, Texture *texture, GraphicsDevice *device)
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

		CAGE_FORCE_INLINE Holder<Shader> pickShaderVariant(MultiShader *multiShader, const Model *model, uint32 textureVariant, const RenderModeEnum renderMode, const bool skeletalAnimation)
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

		CAGE_FORCE_INLINE ShaderAnimationComponent getAnimSpeed(Entity *e)
		{
			ShaderAnimationComponent anim = e->getOrDefault<ShaderAnimationComponent>();
			if (!anim.speed.valid())
				anim.speed = 1;
			if (!anim.offset.valid())
				anim.offset = 0;
			return anim;
		}

		CAGE_FORCE_INLINE Real animCoefficient(uint64 duration, uint64 currentTime, uint64 startTime, Real animationSpeed, Real animationOffset)
		{
			duration = max(duration, uint64(1));
			const double sample = ((double)((sint64)currentTime - (sint64)startTime) * (double)animationSpeed.value) / (double)duration + (double)animationOffset.value;
			if (sample >= 0 && sample <= 1)
				return sample; // already has sufficient precision and preservers looping or not
			return (sample - std::floor(sample)) + sign(sample); // improve precision and ensure that the value stays outside looping range
		}

		CAGE_FORCE_INLINE UniMesh makeMeshUni(const RenderItem &rd)
		{
			UniMesh uni;
			uni.modelMat = Mat3x4(Mat4(rd->transform));
			uni.color = rd->color;
			uni.animation = rd->animation;
			return uni;
		}

		CAGE_FORCE_INLINE Transform approximateMatrix(Mat4 mat)
		{
			const Vec3 center = Vec3(mat * Vec4(0, 0, 0, 1));
			const Vec3 forward = Vec3(mat * Vec4(0, 0, -1, 1));
			const Vec3 up = Vec3(mat * Vec4(0, 1, 0, 1));
			return Transform(center, Quat(forward - center, up - center), distance(center, forward));
		}

		CAGE_FORCE_INLINE std::optional<SkeletalAnimationComponent> skeletalOptional(SkeletalAnimationComponent src)
		{
			for (const auto &it : src.animations)
				if (it.animation)
					return src;
			return {};
		}

		CAGE_FORCE_INLINE bool emptyMask(Entity *e)
		{
			return e->getOrDefault<SceneComponent>().sceneMask == 0;
		}

		Holder<Texture> createShadowmapCascadeView(Texture *tex, uint32 cascade)
		{
			gpu::TextureViewDescriptor desc = {};
			desc.baseArrayLayer = cascade;
			desc.arrayLayerCount = 1;
			desc.label = "shadowmap cascade view";
			gpu::TextureView view = tex->nativeTexture().createView(desc);
			return newTexture(tex->nativeTexture(), view, {}, "shadowmap cascade view");
		}

		Holder<Texture> createShadowmap2D(uint32 entityId, uint32 resolution, uint32 cascades, GraphicsDevice *device)
		{
			TransientTextureCreateConfig conf;
			conf.name = "shadowmap target";
			conf.resolution = Vec3i(resolution, resolution, cascades);
			conf.format = gpu::TextureFormat::Depth32Float;
			conf.flags = TextureFlags::Array;
			conf.entityId = entityId;
			return newTexture(device, conf);
		}

		Holder<Texture> createShadowmapCube(uint32 entityId, uint32 resolution, GraphicsDevice *device)
		{
			TransientTextureCreateConfig conf;
			conf.name = "shadowmap target";
			conf.resolution = Vec3i(resolution, resolution, 6);
			conf.format = gpu::TextureFormat::Depth16Unorm;
			conf.flags = TextureFlags::Cubemap;
			conf.entityId = entityId;
			return newTexture(device, conf);
		}

		struct RenderBaseBase;

		struct SceneImpl : private Immovable
		{
			const SceneRenderConfig &config;

			Model *modelSquare = nullptr, *modelBone = nullptr, *modelSprite = nullptr;
			Shader *shaderBlitPixels = nullptr, *shaderBlitScaled = nullptr;
			MultiShader *shaderStandard = nullptr, *shaderSprite = nullptr;
			Shader *shaderText = nullptr;
			EntityComponent *transformComponent = nullptr;
			EntityComponent *prevTransformComponent = nullptr;

			ankerl::unordered_dense::set<Holder<void>> sharedAssetsCache;
			Holder<SkeletalAnimationPreparatorCollection> skeletonPreparatorCollection;
			std::vector<Holder<RenderBaseBase>> renderers;
			ItemsContainer<SceneItem> items; // ensure that this does not reallocate once
			RenderBaseBase *distOverride = nullptr;
			const bool cnfRenderMissingModels = confGlobalRenderMissingModels;
			const bool cnfRenderSkeletonBones = confGlobalRenderSkeletonBones;

			// temporary caches
			std::vector<Model *> prepareModelsRaw;
			std::vector<Holder<Model>> prepareModelsHolders;

			SceneImpl(const SceneRenderConfig &config) : config(config)
			{
				transformComponent = config.shared.scene->component<TransformComponent>();
				prevTransformComponent = config.shared.scene->componentsByType(detail::typeIndex<TransformComponent>())[1];
				skeletonPreparatorCollection = newSkeletalAnimationPreparatorCollection(config.shared.assets);
			}

			template<class T>
			CAGE_FORCE_INLINE T *shareAsset(Holder<T> &&asset)
			{
				T *ret = +asset;
				if (asset)
					sharedAssetsCache.insert(std::move(asset).template cast<void>());
				return ret;
			}

			CAGE_FORCE_INLINE Transform modelTransform(Entity *e) const
			{
				CAGE_ASSERT(e->has(transformComponent));
				if (e->has(prevTransformComponent))
				{
					const Transform c = e->value<TransformComponent>(transformComponent);
					const Transform p = e->value<TransformComponent>(prevTransformComponent);
					return interpolate(p, c, config.shared.interpolationFactor);
				}
				else
					return e->value<TransformComponent>(transformComponent);
			}

			void loadBasicAssets()
			{
				modelSquare = shareAsset(config.shared.assets->get<Model>(HashString("cage/models/square.obj")));
				modelBone = shareAsset(config.shared.assets->get<Model>(HashString("cage/models/bone.obj")));
				modelSprite = shareAsset(config.shared.assets->get<Model>(HashString("cage/models/icon.obj")));
				CAGE_ASSERT(modelSquare && modelBone && modelSprite);

				shaderBlitPixels = shareAsset(config.shared.assets->get<MultiShader>(HashString("cage/shaders/engine/blitPixels.glsl"))->get(0));
				shaderBlitScaled = shareAsset(config.shared.assets->get<MultiShader>(HashString("cage/shaders/engine/blitScaled.glsl"))->get(0));
				CAGE_ASSERT(shaderBlitPixels && shaderBlitScaled);

				shaderStandard = shareAsset(config.shared.assets->get<MultiShader>(HashString("cage/shaders/engine/standard.glsl")));
				shaderSprite = shareAsset(config.shared.assets->get<MultiShader>(HashString("cage/shaders/engine/icon.glsl")));
				CAGE_ASSERT(shaderStandard && shaderSprite);

				shaderText = shareAsset(config.shared.assets->get<MultiShader>(HashString("cage/shaders/engine/text.glsl"))->get(0));
				CAGE_ASSERT(shaderText);
			}

			void prepareModelBones(const SceneItem &rd)
			{
				const SceneModel &rm = rd.data.model();
				CAGE_ASSERT(rm.mesh);
				CAGE_ASSERT(rm.skeletalAnimation);
				const auto armature = rm.skeletalAnimation->armature();
				for (uint32 i = 0; i < armature.size(); i++)
				{
					SceneItem d;
					d.transform = rd.transform * approximateMatrix(Mat4(armature[i]));
					d.color = Vec4(colorGammaToLinear(colorHsvToRgb(Vec3(Real(i) / Real(armature.size()), 1, 1))), 1);
					d.e = rd.e;
					d.renderLayer = rd.renderLayer;
					d.shadowCast = false;
					d.orderDependent = false;
					d.blending = false;
					SceneModel r;
					r.mesh = modelBone;
					d.data.assign(std::move(r));
					distribute(std::move(d));
				}
			}

			Holder<SkeletalAnimationPreparatorInstance> prepareSkeleton(void *object, const uint64 startTime, const SkeletalAnimationComponent &ps, const Model *mesh)
			{
				static_assert(std::extent_v<decltype(SkeletalAnimationPreparatorConfig::animations)> == std::extent_v<decltype(SkeletalAnimationComponent::animations)>);
				static_assert(decltype(SkeletalAnimationLayer::maskName)::MaxLength == SkeletalAnimationMaskLabel::MaxLength);

				SkeletonRig *skeleton = nullptr;
				SkeletalAnimationPreparatorConfig cnf;
				for (uint32 i = 0; i < std::extent_v<decltype(SkeletalAnimationPreparatorConfig::animations)>; i++)
				{
					const auto &input = ps.animations[i];
					auto &output = cnf.animations[i];
					if (!input.animation)
						continue;
					output.animation = shareAsset(config.shared.assets->get<AssetSchemeIndexSkeletalAnimation, SkeletalAnimation>(input.animation));
					if (!output.animation)
						return {};
					CAGE_ASSERT(output.animation->bonesCount() == mesh->bonesCount);
					const uint64 time = input.startTime ? input.startTime : startTime;
					output.coefficient = animCoefficient(output.animation->duration, config.shared.currentTime, time, input.speed, input.offset);
					output.weight = input.weight;
					output.blendingMode = input.blendingMode;
					if (!input.maskName.empty())
					{
						if (!skeleton)
							skeleton = shareAsset(config.shared.assets->get<AssetSchemeIndexSkeletonRig, SkeletonRig>(output.animation->skeletonName));
						CAGE_ASSERT(skeleton);
						output.mask = skeleton->namedMask(input.maskName);
					}
				}

				const auto &checkSkeletonIdsAreSame = [&]() -> bool
				{
					std::optional<uint32> id;
					for (const auto &ani : cnf.animations)
					{
						if (!ani.animation)
							continue;
						if (id && *id != ani.animation->skeletonName)
							return false;
						id = ani.animation->skeletonName;
					}
					return true;
				};
				CAGE_ASSERT(checkSkeletonIdsAreSame());

				cnf.modelImportTransform = mesh->importTransform;
				cnf.object = object;
				cnf.animateSkeletonsInsteadOfSkins = cnfRenderSkeletonBones;
				return skeletonPreparatorCollection->create(std::move(cnf));
			}

			void prepareModel(SceneItem &rd, const RenderObject *parent = {})
			{
				SceneModel &rm = rd.data.model();
				CAGE_ASSERT(rm.mesh);

				std::optional<SkeletalAnimationComponent> ps;
				if (rd.e->has<SkeletalAnimationComponent>())
					ps = skeletalOptional(rd.e->value<SkeletalAnimationComponent>());

				ModelComponent render = rd.e->value<ModelComponent>();
				ColorComponent color = rd.e->getOrDefault<ColorComponent>();
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
						ps = SkeletalAnimationComponent().add({ .animation = parent->skelAnimId });
				}

				if (!rm.mesh->bonesCount)
					ps.reset();
				if (ps)
				{
					rm.skeletalAnimation = prepareSkeleton(rd.e, startTime, *ps, rm.mesh);
					if (!rm.skeletalAnimation)
						ps.reset();
				}

				const ShaderAnimationComponent anim = getAnimSpeed(rd.e);
				rd.color = initializeColor(color);
				rd.animation = Vec4((double)(sint64)(config.shared.currentTime - startTime) / (double)1'000'000, anim.speed, anim.offset, 0);
				rd.renderLayer = render.renderLayer + rm.mesh->renderLayer;
				rd.shadowCast = any(rm.mesh->renderFlags & MeshRenderFlags::ShadowCast);
				rd.orderDependent = rd.blending = any(rm.mesh->renderFlags & (MeshRenderFlags::Transparent | MeshRenderFlags::Fade)) || rd.color[3] < 1;
				rd.orderDependent &= none(rm.mesh->renderFlags & MeshRenderFlags::OrderIndependent);

				if (rm.skeletalAnimation && cnfRenderSkeletonBones)
					prepareModelBones(rd);
				else
					distribute(std::move(rd));
			}

			void prepareSprite(Entity *e, Texture *tex, Model *mesh)
			{
				const SpriteComponent &ic = e->value<SpriteComponent>();
				SceneSprite ri;
				ri.texture = tex;
				ri.mesh = mesh;
				CAGE_ASSERT(ri.mesh->bonesCount == 0);
				SceneItem rd;
				rd.e = e;
				rd.transform = modelTransform(e);
				rd.color = initializeColor(e->getOrDefault<ColorComponent>());
				const uint64 startTime = e->getOrDefault<SpawnTimeComponent>().spawnTime;
				const ShaderAnimationComponent anim = getAnimSpeed(e);
				rd.animation = Vec4((double)(sint64)(config.shared.currentTime - startTime) / (double)1'000'000, anim.speed, anim.offset, 0);
				rd.renderLayer = ic.renderLayer + ri.mesh->renderLayer;
				rd.shadowCast = any(ri.mesh->renderFlags & MeshRenderFlags::ShadowCast);
				rd.orderDependent = rd.blending = any(ri.mesh->renderFlags & (MeshRenderFlags::Transparent | MeshRenderFlags::Fade)) || rd.color[3] < 1;
				rd.orderDependent &= none(ri.mesh->renderFlags & MeshRenderFlags::OrderIndependent);
				rd.data.assign(std::move(ri));
				distribute(std::move(rd));
			}

			void prepareText(Entity *e, TextComponent tc)
			{
				if (!tc.fontId)
					tc.fontId = detail::GuiTextFontDefault;
				if (!tc.fontId)
					tc.fontId = HashString("cage/fonts/ubuntu/regular.ttf");
				SceneText rt;
				rt.font = shareAsset(config.shared.assets->get<AssetSchemeIndexFont, Font>(tc.fontId));
				if (!rt.font)
					return;
				FontFormat format;
				format.size = 1;
				format.align = tc.align;
				format.lineSpacing = tc.lineSpacing;
				const String str = textsGet(tc.textId, e->getOrDefault<TextValueComponent>().value);
				rt.layout = systemMemory().createHolder<FontLayoutResult>(rt.font->layout(str, format));
				if (rt.layout->glyphs.empty())
					return;
				SceneItem rd;
				rd.e = e;
				rd.transform = modelTransform(e) * Transform(Vec3(rt.layout->size * Vec2(-0.5, 0.5), 0));
				rd.color = initializeColor(e->getOrDefault<ColorComponent>());
				rd.renderLayer = tc.renderLayer;
				rd.shadowCast = false;
				rd.orderDependent = true;
				rd.blending = true;
				rd.data.assign(std::move(rt));
				distribute(std::move(rd));
			}

			void prepareCustomDraw(Entity *e, const CustomDrawComponent &cdc)
			{
				CAGE_ASSERT(cdc.callback);
				SceneCustom rc;
				SceneItem rd;
				rd.e = e;
				rd.transform = modelTransform(e);
				rd.color = initializeColor(e->getOrDefault<ColorComponent>());
				rd.renderLayer = cdc.renderLayer;
				rd.shadowCast = false;
				rd.orderDependent = true;
				rd.blending = false;
				rd.data.assign(std::move(rc));
				distribute(std::move(rd));
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
				data.reserve(config.shared.scene->component<ModelComponent>()->count());

				entitiesVisitor(
					[&](Entity *e, const ModelComponent &rc)
					{
						if (!rc.modelId || emptyMask(e))
							return;
						data.push_back({ e, rc.modelId });
					},
					+config.shared.scene, false);
				profiling.set(Stringizer() + "models: " + data.size());
				std::sort(data.begin(), data.end(), [](const Data &a, const Data &b) { return a.id < b.id; });

				const auto &mark = [](const Data &data) { return data.id; };
				const auto &output = [&](PointerRange<const Data> data)
				{
					CAGE_ASSERT(data.size() > 0);

					if (Holder<RenderObject> obj = config.shared.assets->get<AssetSchemeIndexRenderObject, RenderObject>(data[0].id))
					{
						CAGE_ASSERT(obj->lodsCount() > 0);
						if (obj->lodsCount() == 1)
						{
							prepareModelsRaw.clear();
							for (uint32 id : obj->models(0))
							{
								if (Model *mesh = shareAsset(config.shared.onDemand->get<AssetSchemeIndexModel, Model>(id)))
									prepareModelsRaw.push_back(std::move(mesh));
							}
							if (prepareModelsRaw.empty())
								return;
							for (const auto &it : data)
							{
								const Transform tr = modelTransform(it.e);
								for (const auto &mesh : prepareModelsRaw)
								{
									SceneModel rm;
									rm.mesh = mesh;
									SceneItem rd;
									rd.e = it.e;
									rd.transform = tr;
									rd.data.assign(std::move(rm));
									prepareModel(rd, +obj);
								}
							}
							return;
						}

						// we must direct the objects individually into all cameras/shadowmaps
						RenderObject *o = shareAsset(std::move(obj));
						for (const auto &it : data)
							prepareEntitiesModelsLod(it.e, o);
						return;
					}

					if (Model *mesh = shareAsset(config.shared.assets->get<AssetSchemeIndexModel, Model>(data[0].id)))
					{
						for (const auto &it : data)
						{
							SceneModel rm;
							rm.mesh = mesh;
							SceneItem rd;
							rd.e = it.e;
							rd.transform = modelTransform(it.e);
							rd.data.assign(std::move(rm));
							prepareModel(rd);
						}
						return;
					}

					if (cnfRenderMissingModels)
					{
						Model *mesh = shareAsset(config.shared.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/fake.obj")));
						if (!mesh)
							return;
						for (const auto &it : data)
						{
							SceneModel rm;
							rm.mesh = mesh;
							SceneItem rd;
							rd.e = it.e;
							rd.transform = modelTransform(it.e);
							rd.data.assign(std::move(rm));
							prepareModel(rd);
						}
					}
				};
				partition(PointerRange<const Data>(data), mark, output);
			}

			void prepareEntitiesSprites()
			{
				ProfilingScope profiling("sprites");

				struct Data
				{
					SpriteComponent ic;
					Entity *e = nullptr;
				};
				std::vector<Data> data;
				data.reserve(config.shared.scene->component<SpriteComponent>()->count());

				entitiesVisitor(
					[&](Entity *e, const SpriteComponent &ic)
					{
						if (!ic.spriteId || emptyMask(e))
							return;
						data.push_back({ ic, e });
					},
					+config.shared.scene, false);
				profiling.set(Stringizer() + "sprites: " + data.size());
				std::sort(data.begin(), data.end(), [](const Data &a, const Data &b) { return std::pair{ a.ic.spriteId, a.ic.modelId } < std::pair{ b.ic.spriteId, b.ic.modelId }; });

				const auto &mark = [](const Data &data) { return std::pair{ data.ic.spriteId, data.ic.modelId }; };
				const auto &output = [&](PointerRange<const Data> data)
				{
					CAGE_ASSERT(data.size() > 0);
					Texture *tex = shareAsset(config.shared.assets->get<AssetSchemeIndexTexture, Texture>(data[0].ic.spriteId));
					if (!tex)
						return;
					Model *mesh = data[0].ic.modelId ? shareAsset(config.shared.assets->get<AssetSchemeIndexModel, Model>(data[0].ic.modelId)) : modelSprite;
					if (!mesh)
						return;
					for (const auto &it : data)
						prepareSprite(it.e, tex, mesh);
				};
				partition(PointerRange<const Data>(data), mark, output);
			}

			void prepareEntities()
			{
				ProfilingScope profiling("prepare entities");
				profiling.set(Stringizer() + "entities: " + config.shared.scene->count());

				prepareEntitiesModels();
				prepareEntitiesSprites();

				entitiesVisitor(
					[&](Entity *e, const TextComponent &tc)
					{
						if (emptyMask(e))
							return;
						prepareText(e, tc);
					},
					+config.shared.scene, false);

				entitiesVisitor(
					[&](Entity *e, const CustomDrawComponent &cdc)
					{
						if (emptyMask(e))
							return;
						prepareCustomDraw(e, cdc);
					},
					+config.shared.scene, false);
			}

			void prepareEntitiesModelsLod(Entity *e, RenderObject *object);
			void distribute(SceneItem &&item);
			void generateRenderers();
			void dispatchRenders();
		};

		struct ShadowRender;
		struct CameraRender;

		struct RenderBaseBase : private Immovable
		{
			const SceneImpl &scene;
			const SceneRenderCamera &camera;
			bool isShadowmap = false;

			Mat4 model;
			Mat4 view;
			Mat4 viewProj;
			Frustum frustum;
			Holder<GraphicsEncoder> encoder;
			Holder<GraphicsAggregateBuffer> aggregate;
			Holder<GraphicsBuffer> buffViewport, buffProjection;

			ItemsContainer<RenderItem> items;

			// temporary caches
			std::vector<UniMesh> uniMeshes;
			std::vector<Mat3x4> uniArmatures;
			std::vector<float> uniCustomData;

			RenderBaseBase(const SceneImpl &scene, const SceneRenderCamera &camera) : scene(scene), camera(camera) {}

			virtual ~RenderBaseBase() {}

			virtual void entry() = 0;

			CAGE_FORCE_INLINE bool failedMask(Entity *e) const
			{
				const uint32 c = e->getOrDefault<SceneComponent>().sceneMask;
				return (c & camera.cameraSceneMask) == 0;
			}

			CAGE_FORCE_INLINE void appendShaderCustomData(Entity *e, uint32 customDataCount)
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

			void frustumCullItems()
			{
				ProfilingScope profiling("frustum cull items");
				items.erase_if(
					[this](const RenderItem &it) -> bool
					{
						if (it->data.index != VariantEnum::Model)
							return false;
						const auto &mod = it->data.model();
						return !intersects(mod.mesh->boundingBox * it->transform, frustum);
					});
			}

			void orderItems()
			{
				ProfilingScope profiling("order items");
				profiling.set(Stringizer() + "count: " + items.size());

				for (RenderItem &it : items)
				{
					if (it->orderDependent)
						it.depth = (viewProj * Vec4(it->transform.position, 1))[2] * -1;
				}

				std::sort(items.begin(), items.end(), [](const RenderItem &a, const RenderItem &b) { return a.cmp() < b.cmp(); });
			}

			void orderInstances(PointerRange<RenderItem> insts)
			{
				for (RenderItem &it : insts)
					it.depth = (viewProj * Vec4(it->transform.position, 1))[2] * -1;
				std::sort(insts.begin(), insts.end(), [](const RenderItem &a, const RenderItem &b) { return a.depth < b.depth; });
			}
		};

		template<class CrtpDerived>
		struct RenderBase : RenderBaseBase
		{
			RenderBase(const SceneImpl &scene, const SceneRenderCamera &camera) : RenderBaseBase(scene, camera) {}

			CAGE_FORCE_INLINE CrtpDerived *derived() { return static_cast<CrtpDerived *>(this); }

			void renderModels(const RenderModeEnum renderMode, PointerRange<RenderItem> instances)
			{
				CAGE_ASSERT(!instances.empty());
				const RenderItem &rd = instances[0];
				const SceneModel &rm = rd->data.model();

				if constexpr (std::is_same_v<CrtpDerived, CameraRender>)
				{
					if (renderMode == RenderModeEnum::DepthPrepass && none(rm.mesh->renderFlags & MeshRenderFlags::DepthWrite))
						return;

					if (rd->blending && !rd->orderDependent)
						orderInstances(instances);
				}

				const auto material = newGraphicsBindings(scene.config.shared.device, scene.config.shared.assets, +rm.mesh);

				Holder<MultiShader> multiShader = rm.mesh->shaderName ? scene.config.shared.assets->get<AssetSchemeIndexShader, MultiShader>(rm.mesh->shaderName) : Holder<MultiShader>(scene.shaderStandard, nullptr);
				Holder<Shader> shader = pickShaderVariant(+multiShader, rm.mesh, textureShaderVariant(material.second), renderMode, !!rm.skeletalAnimation);

				UniOptions uniOptions;
				{
					if constexpr (std::is_same_v<CrtpDerived, CameraRender>)
					{
						if (renderMode == RenderModeEnum::Color)
						{
							uniOptions.optsLights[0] = derived()->lightsCount;
							uniOptions.optsLights[1] = derived()->shadowedLightsCount;
						}
					}
					const bool ssao = !rd->blending && any(rm.mesh->renderFlags & MeshRenderFlags::DepthWrite) && any(camera.effects.effects & ScreenSpaceEffectsFlags::AmbientOcclusion);
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
					const SceneModel &r = inst->data.model();
					CAGE_ASSERT(+r.mesh == +rm.mesh);
					CAGE_ASSERT(!!r.skeletalAnimation == !!rm.skeletalAnimation);
					uniMeshes.push_back(makeMeshUni(inst));
					if (rm.skeletalAnimation)
					{
						const auto a = r.skeletalAnimation->armature();
						CAGE_ASSERT(a.size() == rm.mesh->bonesCount);
						for (const auto &b : a)
							uniArmatures.push_back(b);
					}
					appendShaderCustomData(inst->e, multiShader->customDataCount);
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
					draw.blending = rd->blending ? BlendingEnum::PremultipliedTransparency : BlendingEnum::None;
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
				draw.material = material.first;
				draw.bindings = newGraphicsBindings(scene.config.shared.device, bind);
				draw.instances = instances.size();
				encoder->draw(draw);
			}

			void renderSprites(const RenderModeEnum renderMode, PointerRange<RenderItem> instances)
			{
				CAGE_ASSERT(!instances.empty());
				const RenderItem &rd = instances[0];

				if constexpr (std::is_same_v<CrtpDerived, CameraRender>)
				{
					if (renderMode != RenderModeEnum::Color)
						return;

					if (rd->blending && !rd->orderDependent)
						orderInstances(instances);
				}

				Model *mesh = rd->data.sprite().mesh;
				Texture *texture = rd->data.sprite().texture;

				Holder<MultiShader> multiShader = mesh->shaderName ? scene.config.shared.assets->get<MultiShader>(mesh->shaderName) : Holder<MultiShader>(scene.shaderSprite, nullptr);
				Holder<Shader> shader = pickShaderVariant(+multiShader, mesh, textureShaderVariant(texture->flags | (TextureFlags)(1u << 31)), renderMode, false);

				UniOptions uniOptions;

				uniMeshes.clear();
				uniMeshes.reserve(instances.size());
				CAGE_ASSERT(multiShader->customDataCount <= 16);
				uniCustomData.clear();
				if (multiShader->customDataCount)
					uniCustomData.reserve(multiShader->customDataCount * instances.size());

				for (const auto &inst : instances)
				{
					CAGE_ASSERT(inst->data.sprite().mesh == mesh);
					CAGE_ASSERT(inst->data.sprite().texture == texture);
					uniMeshes.push_back(makeMeshUni(inst));
					appendShaderCustomData(inst->e, multiShader->customDataCount);
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
					bind.buffers.push_back({ privat::getBufferDummy(scene.config.shared.device), 2 });
				}
				{
					const auto ab = aggregate->writeArray<float>(uniCustomData, 3, false);
					bind.buffers.push_back(ab);
					draw.dynamicOffsets.push_back(ab);
				}

				draw.depthTest = any(mesh->renderFlags & MeshRenderFlags::DepthTest) ? DepthTestEnum::LessEqual : DepthTestEnum::Always;
				draw.depthWrite = any(mesh->renderFlags & MeshRenderFlags::DepthWrite);
				draw.blending = rd->blending ? BlendingEnum::AlphaTransparency : BlendingEnum::None;
				draw.backFaceCulling = none(mesh->renderFlags & MeshRenderFlags::TwoSided);
				draw.model = +mesh;
				draw.shader = +shader;
				draw.material = newGraphicsBindings(scene.config.shared.device, spritesMaterialBinding(mesh, texture, scene.config.shared.device));
				draw.bindings = newGraphicsBindings(scene.config.shared.device, bind);
				draw.instances = instances.size();
				encoder->draw(draw);
			}

			void renderTexts(const RenderModeEnum renderMode, PointerRange<RenderItem> instances)
			{
				CAGE_ASSERT(!instances.empty());

				if constexpr (std::is_same_v<CrtpDerived, CameraRender>)
				{
					if (renderMode != RenderModeEnum::Color)
						return;
				}

				for (const RenderItem &it : instances)
				{
					FontRenderConfig cfg;
					cfg.transform = viewProj * Mat4(it->transform);
					cfg.color = it->color;
					cfg.encoder = +encoder;
					cfg.aggregate = +aggregate;
					cfg.assets = scene.config.shared.onDemand;
					cfg.depthTest = true;
					cfg.guiShader = false;
					const SceneText &t = it->data.text();
					t.font->render(*t.layout, cfg);
				}
			}

			void renderCustom(const RenderModeEnum renderMode, PointerRange<RenderItem> instances)
			{
				CAGE_ASSERT(!instances.empty());
				const RenderItem &rd = instances[0];
				rd->data.custom(); // assert the type

				if constexpr (std::is_same_v<CrtpDerived, CameraRender>)
				{
					if (renderMode != RenderModeEnum::Color)
						return;
				}

				for (const RenderItem &it : instances)
				{
					const ProfilingScope profiling("custom draw");
					it->data.custom(); // assert type
					CustomDrawConfig cfg;
					cfg.sceneConfig = &scene.config.shared;
					cfg.cameraConfig = &camera;
					cfg.entity = it->e;
					cfg.encoder = +encoder;
					cfg.aggregate = +aggregate;
					CAGE_ASSERT(it->e->has<CustomDrawComponent>());
					it->e->value<CustomDrawComponent>().callback(cfg);
				}
			}

			void renderInstances(const RenderModeEnum renderMode, PointerRange<RenderItem> instances)
			{
				CAGE_ASSERT(!instances.empty());
				const RenderItem &rd = instances[0];

				if constexpr (std::is_same_v<CrtpDerived, CameraRender>)
				{
					if (renderMode == RenderModeEnum::DepthPrepass && rd->blending)
						return;
				}

				switch (rd->data.index)
				{
					case VariantEnum::None:
						break;
					case VariantEnum::Model:
						renderModels(renderMode, instances);
						break;
					case VariantEnum::Sprite:
						renderSprites(renderMode, instances);
						break;
					case VariantEnum::Text:
						renderTexts(renderMode, instances);
						break;
					case VariantEnum::Custom:
						renderCustom(renderMode, instances);
						break;
				}
			}

			void renderPass(const RenderModeEnum renderMode)
			{
				CAGE_ASSERT((std::is_same_v<CrtpDerived, ShadowRender> == (renderMode == RenderModeEnum::Shadowmap)));
				const auto &render = [&](PointerRange<RenderItem> instances) { renderInstances(renderMode, instances); };
				partition(PointerRange<RenderItem>(items), [](const RenderItem &r) { return r.mark(); }, render);
			}
		};

		struct ShadowRender : public RenderBase<ShadowRender>
		{
			UniShadowedLight shadowUni;
			Mat4 shadowProjection;
			LightComponent lightComponent;
			ShadowmapComponent shadowmapComponent;
			Holder<Texture> shadowTexture;
			uint32 cascade = 0;

			ShadowRender(const SceneImpl &scene, const SceneRenderCamera &camera) : RenderBase(scene, camera) { isShadowmap = true; }

			void initializeShadowmapCascade()
			{
				CAGE_ASSERT(lightComponent.lightType == LightTypeEnum::Directional);
				CAGE_ASSERT(camera.camera.shadowmapFrustumDepthFraction > 0 && camera.camera.shadowmapFrustumDepthFraction <= 1);

				const auto &sc = shadowmapComponent;
				CAGE_ASSERT(sc.cascadesSplitLogFactor >= 0 && sc.cascadesSplitLogFactor <= 1);
				CAGE_ASSERT(sc.cascadesCount > 0 && sc.cascadesCount <= 4);
				CAGE_ASSERT(sc.cascadesPaddingDistance >= 0);

				// split the frustum of the original camera to find the slice for this cascade

				const Mat4 invP = inverse(camera.projection);
				const auto &getPlane = [&](Real ndcZ) -> Real
				{
					const Vec4 a = invP * Vec4(0, 0, ndcZ, 1);
					return -a[2] / a[3];
				};
				const Real cameraNear = getPlane(0);
				const Real cameraFar = getPlane(1) * camera.camera.shadowmapFrustumDepthFraction;
				CAGE_ASSERT(cameraNear > 0 && cameraFar > cameraNear);

				const auto &getSplit = [&](Real f) -> Real
				{
					const Real uniform = cameraNear + (cameraFar - cameraNear) * f;
					const Real log = cameraNear * pow(cameraFar / cameraNear, f);
					return interpolate(uniform, log, sc.cascadesSplitLogFactor);
				};
				const Real splitNear = getSplit(Real(cascade + 0) / sc.cascadesCount);
				const Real splitFar = getSplit(Real(cascade + 1) / sc.cascadesCount);
				CAGE_ASSERT(splitNear > 0 && splitFar > splitNear);

				const auto viewZtoNdcZ = [&](Real viewZ) -> Real
				{
					const Vec4 a = camera.projection * Vec4(0, 0, -viewZ, 1);
					return a[2] / a[3];
				};
				const Real splitNearNdc = viewZtoNdcZ(splitNear);
				const Real splitFarNdc = viewZtoNdcZ(splitFar);

				const Mat4 invVP = inverse(camera.projection * Mat4(inverse(camera.transform)));
				const auto &getPoint = [&](Vec3 p) -> Vec3
				{
					const Vec4 a = invVP * Vec4(p, 1);
					return Vec3(a) / a[3];
				};
				// world-space corners of the camera frustum slice
				const std::array<Vec3, 8> corners = { getPoint(Vec3(-1, -1, splitNearNdc)), getPoint(Vec3(1, -1, splitNearNdc)), getPoint(Vec3(1, 1, splitNearNdc)), getPoint(Vec3(-1, 1, splitNearNdc)), getPoint(Vec3(-1, -1, splitFarNdc)), getPoint(Vec3(1, -1, splitFarNdc)), getPoint(Vec3(1, 1, splitFarNdc)), getPoint(Vec3(-1, 1, splitFarNdc)) };

				const Vec3 lightDir = Vec3(model * Vec4(0, 0, -1, 0));
				const Vec3 lightUp = abs(dot(lightDir, Vec3(0, 1, 0))) > 0.99 ? Vec3(0, 0, 1) : Vec3(0, 1, 0);

				Aabb worldBox;
				for (Vec3 wp : corners)
					worldBox += Aabb(wp);

				Vec3 eye = worldBox.center();
				view = Mat4(inverse(Transform(eye, Quat(lightDir, lightUp))));

				// determine the bounding box of the shadow volume

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

				// adjust the shadow near and far planes to not exclude preceding shadow casters
				shadowBox.a[2] -= sc.cascadesPaddingDistance;
				shadowBox.b[2] += sc.cascadesPaddingDistance;

				// move far plane much further, to improve precision
				const Real currentDepth = -shadowBox.a[2] + shadowBox.b[2];
				shadowBox.a[2] -= currentDepth * 10;

				shadowProjection = orthographicProjection(shadowBox.a[0], shadowBox.b[0], shadowBox.a[1], shadowBox.b[1], -shadowBox.b[2], -shadowBox.a[2]);
				viewProj = shadowProjection * view;
				frustum = Frustum(viewProj);
				static constexpr Mat4 bias = Mat4(0.5, 0, 0, 0, 0, -0.5, 0, 0, 0, 0, 1, 0, 0.5, 0.5, 0, 1);
				shadowUni.shadowMat[cascade] = bias * viewProj;
				shadowUni.cascadesDepths[cascade] = splitFar;
			}

			void initializeShadowmapSingle()
			{
				CAGE_ASSERT(cascade == 0);

				view = inverse(model);
				shadowProjection = [&]()
				{
					const auto lc = lightComponent;
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

				viewProj = shadowProjection * view;
				frustum = Frustum(viewProj);
				static constexpr Mat4 bias = Mat4(0.5, 0, 0, 0, 0, -0.5, 0, 0, 0, 0, 1, 0, 0.5, 0.5, 0, 1);
				shadowUni.shadowMat[cascade] = bias * viewProj;
			}

			void entry() override
			{
				frustumCullItems();
				orderItems();

				encoder = newGraphicsEncoder(scene.config.shared.device, "scene render shadowmap");
				aggregate = newGraphicsAggregateBuffer({ scene.config.shared.device });

				{
					UniProjection uni;
					uni.viewMat = view;
					uni.projMat = shadowProjection;
					uni.vpMat = viewProj;
					uni.vpInv = inverse(viewProj);
					buffProjection = newGraphicsBuffer(scene.config.shared.device, sizeof(uni), "UniProjection");
					buffProjection->writeStruct(uni);
				}

				GraphicsBindings globalBindings;
				{
					GraphicsBuffer *dummyStorage = privat::getBufferDummy(scene.config.shared.device);
					Texture *dummyRegular = privat::getTextureDummy2d(scene.config.shared.device);
					GraphicsBindingsCreateConfig bind;
					bind.buffers.push_back({ .buffer = +buffViewport, .binding = 0, .uniform = true });
					bind.buffers.push_back({ .buffer = +buffProjection, .binding = 1, .uniform = true });
					bind.buffers.push_back({ dummyStorage, 2 });
					bind.buffers.push_back({ dummyStorage, 3 });
					bind.textures.push_back({ dummyRegular, 4 });
					bind.textures.push_back({ dummyRegular, 6 });
					globalBindings = newGraphicsBindings(scene.config.shared.device, bind);
				}

				Holder<Texture> target = createShadowmapCascadeView(+shadowTexture, cascade);

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
		};

		struct CameraRender : public RenderBase<CameraRender>
		{
			std::vector<Holder<ShadowRender>> shadowmaps;
			Holder<GraphicsBuffer> buffLights, buffShadowedLights;
			uint32 lightsCount = 0;
			uint32 shadowedLightsCount = 0;

			CameraRender(const SceneImpl &scene, const SceneRenderCamera &camera) : RenderBase(scene, camera) {}

			// must be executed before creating shadowmap renderers
			void initialize()
			{
				model = Mat4(camera.transform);
				view = inverse(model);
				viewProj = camera.projection * view;

				{
					UniViewport viewport;
					viewport.viewMat = view;
					viewport.eyePos = model * Vec4(0, 0, 0, 1);
					viewport.eyeDir = model * Vec4(0, 0, -1, 0);
					viewport.ambientLight = Vec4(colorGammaToLinear(camera.camera.ambientColor) * camera.camera.ambientIntensity, 0);
					viewport.skyLight = Vec4(colorGammaToLinear(camera.camera.skyColor) * camera.camera.skyIntensity, 0);
					viewport.viewport = Vec4(Vec2(), Vec2(camera.resolution));
					viewport.time = Vec4(scene.config.shared.frameIndex % 10'000, (scene.config.shared.currentTime % uint64(1e6)) / 1e6, (scene.config.shared.currentTime % uint64(1e9)) / 1e9, 0);
					buffViewport = newGraphicsBuffer(scene.config.shared.device, sizeof(viewport), "UniViewport");
					buffViewport->writeStruct(viewport);
				}

				{
					UniProjection uni;
					uni.viewMat = view;
					uni.projMat = camera.projection;
					uni.vpMat = viewProj;
					uni.vpInv = inverse(viewProj);
					buffProjection = newGraphicsBuffer(scene.config.shared.device, sizeof(uni), "UniProjection");
					buffProjection->writeStruct(uni);
				}
			}

			Holder<ShadowRender> createShadowRenderBase(Entity *e, const LightComponent &lc, const ShadowmapComponent &sc)
			{
				CAGE_ASSERT(e->id() != 0); // lights with shadowmap may not be anonymous

				Holder<ShadowRender> data = systemMemory().createHolder<ShadowRender>(scene, camera);

				data->lightComponent = lc;
				data->shadowmapComponent = sc;
				data->buffViewport = buffViewport.share();

				{ // quantize light direction - reduces shimmering of slowly rotating lights
					CAGE_ASSERT(e->has<TransformComponent>());
					Transform src = scene.modelTransform(e);
					Vec3 f = src.orientation * Vec3(0, 0, -1);
					f *= 1'000;
					for (uint32 i = 0; i < 3; i++)
						f[i] = round(f[i]);
					f = normalize(f);
					src.orientation = Quat(f, src.orientation * Vec3(0, 1, 0));
					data->model = Mat4(src);
				}

				{
					UniShadowedLight &uni = data->shadowUni;
					(UniLight &)uni = initializeLightUni(data->model, lc, e->getOrDefault<ColorComponent>(), data->camera.transform.position);
					uni.shadowParams[2] = sc.normalOffsetScale;
					uni.shadowParams[3] = sc.shadowFactor;
					uni.params[0] += 1; // shadowed light type
				}

				shadowmaps.push_back(data.share());
				return data;
			}

			void createShadowRender(std::vector<Holder<RenderBaseBase>> &output, Entity *e, const LightComponent &lc, const ShadowmapComponent &sc)
			{
				switch (lc.lightType)
				{
					case LightTypeEnum::Directional:
					{
						CAGE_ASSERT(sc.cascadesCount > 0 && sc.cascadesCount <= 4);
						Holder<Texture> tex = createShadowmap2D(e->id(), sc.resolution, sc.cascadesCount, scene.config.shared.device);
						ShadowRender *first = nullptr;
						for (uint32 cascade = 0; cascade < sc.cascadesCount; cascade++)
						{
							Holder<ShadowRender> data = createShadowRenderBase(e, lc, sc);
							data->cascade = cascade;
							data->shadowTexture = tex.share();
							data->initializeShadowmapCascade();
							if (cascade == 0)
								first = +data;
							else
							{
								first->shadowUni.cascadesDepths[cascade] = data->shadowUni.cascadesDepths[cascade];
								first->shadowUni.shadowMat[cascade] = data->shadowUni.shadowMat[cascade];
							}
							output.push_back(std::move(data).cast<RenderBaseBase>());
						}
						break;
					}
					case LightTypeEnum::Spot:
					case LightTypeEnum::Point:
					{
						Holder<ShadowRender> data = createShadowRenderBase(e, lc, sc);
						data->initializeShadowmapSingle();
						if (lc.lightType == LightTypeEnum::Point)
							data->shadowTexture = createShadowmapCube(e->id(), sc.resolution, scene.config.shared.device);
						else
							data->shadowTexture = createShadowmap2D(e->id(), sc.resolution, 1, scene.config.shared.device);
						output.push_back(std::move(data).cast<RenderBaseBase>());
						break;
					}
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid light type");
				}
			}

			void entry() override
			{
				frustum = Frustum(viewProj);
				frustumCullItems();
				orderItems();
				prepareLights();

				encoder = newGraphicsEncoder(scene.config.shared.device, "scene render camera");
				aggregate = newGraphicsAggregateBuffer({ scene.config.shared.device });

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
					for (const auto &sh : shadowmaps)
					{
						if (sh->cascade != 0)
							continue;
						const uint32 index = numeric_cast<uint32>(sh->shadowUni.shadowParams[0]);
						CAGE_ASSERT(index < 8);
						switch (sh->lightComponent.lightType)
						{
							case LightTypeEnum::Directional:
							case LightTypeEnum::Spot:
								CAGE_ASSERT(!sh2d[index]);
								sh2d[index] = +sh->shadowTexture;
								break;
							case LightTypeEnum::Point:
								CAGE_ASSERT(!shCube[index]);
								shCube[index] = +sh->shadowTexture;
								break;
						}
					}
					for (uint32 i = 0; i < 8; i++)
					{
						if (!sh2d[i])
							sh2d[i] = privat::getTextureDummyArray(scene.config.shared.device);
						if (!shCube[i])
							shCube[i] = privat::getTextureDummyCube(scene.config.shared.device);
					}

					for (uint32 i = 0; i < 8; i++)
					{
						bind.textures.push_back(GraphicsBindingsCreateConfig::TextureBindingConfig{ .texture = sh2d[i], .binding = 16 + i, .bindSampler = false });
						bind.textures.push_back(GraphicsBindingsCreateConfig::TextureBindingConfig{ .texture = shCube[i], .binding = 24 + i, .bindSampler = false });
					}

					// shadowmap sampler
					bind.textures.push_back(GraphicsBindingsCreateConfig::TextureBindingConfig{ .texture = privat::getTextureShadowsSampler(scene.config.shared.device), .binding = 15, .bindTexture = false });

					globalBindings = newGraphicsBindings(scene.config.shared.device, bind);
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
					drawcfg.model = scene.modelSquare;
					drawcfg.shader = scene.shaderBlitPixels;
					GraphicsBindingsCreateConfig bind;
					bind.textures.push_back({ +depthTexture, 0 });
					drawcfg.bindings = newGraphicsBindings(scene.config.shared.device, bind);
					encoder->draw(drawcfg);
				}

				ScreenSpaceCommonConfig commonConfig; // helper to simplify initialization
				commonConfig.assets = scene.config.shared.assets;
				commonConfig.encoder = +encoder;
				commonConfig.aggregate = +aggregate;
				commonConfig.resolution = camera.resolution;

				// ssao
				if (any(camera.effects.effects & ScreenSpaceEffectsFlags::AmbientOcclusion))
				{
					ScreenSpaceAmbientOcclusionConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					(ScreenSpaceAmbientOcclusion &)cfg = camera.effects.ssao;
					cfg.proj = camera.projection;
					cfg.inDepth = +depthSampling;
					cfg.outAo = +ssaoTexture;
					cfg.frameIndex = scene.config.shared.frameIndex;
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
					if (any(camera.effects.effects & ScreenSpaceEffectsFlags::DepthOfField))
					{
						ScreenSpaceDepthOfFieldConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						(ScreenSpaceDepthOfField &)cfg = camera.effects.depthOfField;
						cfg.proj = camera.projection;
						cfg.inDepth = +depthTexture;
						cfg.inColor = +texSource;
						cfg.outColor = +texTarget;
						screenSpaceDepthOfField(cfg);
						std::swap(texSource, texTarget);
					}

					// bloom
					if (any(camera.effects.effects & ScreenSpaceEffectsFlags::Bloom))
					{
						ScreenSpaceBloomConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						(ScreenSpaceBloom &)cfg = camera.effects.bloom;
						cfg.inColor = +texSource;
						cfg.outColor = +texTarget;
						screenSpaceBloom(cfg);
						std::swap(texSource, texTarget);
					}

					// tonemap & gamma correction
					if (any(camera.effects.effects & (ScreenSpaceEffectsFlags::ToneMapping | ScreenSpaceEffectsFlags::GammaCorrection)))
					{
						ScreenSpaceTonemapConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						cfg.inColor = +texSource;
						cfg.outColor = +texTarget;
						cfg.gamma = any(camera.effects.effects & ScreenSpaceEffectsFlags::GammaCorrection) ? camera.effects.gamma : 1;
						cfg.tonemapEnabled = any(camera.effects.effects & ScreenSpaceEffectsFlags::ToneMapping);
						screenSpaceTonemap(cfg);
						std::swap(texSource, texTarget);
					}

					// fxaa
					if (any(camera.effects.effects & ScreenSpaceEffectsFlags::AntiAliasing))
					{
						ScreenSpaceFastApproximateAntiAliasingConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						cfg.inColor = +texSource;
						cfg.outColor = +texTarget;
						screenSpaceFastApproximateAntiAliasing(cfg);
						std::swap(texSource, texTarget);
					}

					// sharpening
					if (any(camera.effects.effects & ScreenSpaceEffectsFlags::Sharpening) && camera.effects.sharpening.strength > 1e-3)
					{
						ScreenSpaceSharpeningConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						cfg.inColor = +texSource;
						cfg.outColor = +texTarget;
						cfg.strength = camera.effects.sharpening.strength;
						screenSpaceSharpening(cfg);
						std::swap(texSource, texTarget);
					}

					colorTexture = std::move(texSource);
				}

				// final blit to target

				{
					const ProfilingScope profiling("final blit");
					RenderPassConfig passcfg;
					passcfg.colorTargets.push_back({ camera.target });
					encoder->nextPass(passcfg);
					const auto scope = encoder->namedScope("final blit");
					DrawConfig drawcfg;
					drawcfg.model = scene.modelSquare;
					if (camera.resolution == camera.target->resolution())
						drawcfg.shader = scene.shaderBlitPixels;
					else
						drawcfg.shader = scene.shaderBlitScaled;
					GraphicsBindingsCreateConfig bind;
					bind.textures.push_back({ +colorTexture, 0 });
					drawcfg.bindings = newGraphicsBindings(scene.config.shared.device, bind);
					encoder->draw(drawcfg);
				}

				aggregate->submit();
				aggregate.clear();
			}

			void prepareLights()
			{
				ProfilingScope profiling("prepare lights");

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
							UniLight uni = initializeLightUni(Mat4(scene.modelTransform(e)), lc, e->getOrDefault<ColorComponent>(), camera.transform.position);
							if (lc.lightType == LightTypeEnum::Point && !intersects(Sphere(Vec3(uni.position), lc.maxDistance), frustum))
								return;
							lights.push_back(uni);
						},
						scene.config.shared.scene, false);
					filterLightsOverLimit(lights, camera.camera.maxLights);
					lightsCount = numeric_cast<uint32>(lights.size());
					if (lights.empty())
						lights.resize(1); // must ensure non-empty buffer
					buffLights = newGraphicsBuffer(scene.config.shared.device, lights.size() * sizeof(UniLight), "UniLight");
					buffLights->writeArray<UniLight>(lights);
				}

				{ // add shadowed lights
					std::vector<UniShadowedLight> shadows;
					uint32 tex2dCount = 0, texCubeCount = 0;
					shadows.reserve(10);
					for (auto &sh : shadowmaps)
					{
						if (sh->cascade != 0)
							continue;
						if (sh->lightComponent.lightType == LightTypeEnum::Point)
						{
							if (texCubeCount >= 8)
								CAGE_THROW_ERROR(Exception, "too many shadowmaps (cube shadows)");
							sh->shadowUni.shadowParams[0] = texCubeCount;
							texCubeCount++;
						}
						else
						{
							if (tex2dCount >= 8)
								CAGE_THROW_ERROR(Exception, "too many shadowmaps (2D shadows)");
							sh->shadowUni.shadowParams[0] = tex2dCount;
							tex2dCount++;
						}
						shadows.push_back(sh->shadowUni);
					}
					CAGE_ASSERT(shadows.size() == tex2dCount + texCubeCount);
					shadowedLightsCount = numeric_cast<uint32>(shadows.size());
					if (shadows.empty())
						shadows.resize(1); // must ensure non-empty buffer
					buffShadowedLights = newGraphicsBuffer(scene.config.shared.device, shadows.size() * sizeof(UniShadowedLight), "UniShadowedLight");
					buffShadowedLights->writeArray<UniShadowedLight>(shadows);
				}

				profiling.set(Stringizer() + "count: " + lightsCount + ", shadowed: " + shadowedLightsCount);
			}

			Holder<Texture> createDepthTextureTarget()
			{
				TransientTextureCreateConfig conf;
				conf.name = "depth target";
				conf.resolution = Vec3i(camera.resolution, 1);
				conf.format = gpu::TextureFormat::Depth32Float;
				return newTexture(scene.config.shared.device, conf);
			}

			Holder<Texture> createDepthSamplingTexture()
			{
				TransientTextureCreateConfig conf;
				conf.name = "depth sampling";
				conf.resolution = Vec3i(camera.resolution, 1);
				conf.format = gpu::TextureFormat::R32Float;
				return newTexture(scene.config.shared.device, conf);
			}

			Holder<Texture> createColorTextureTarget(const AssetLabel &name)
			{
				TransientTextureCreateConfig conf;
				conf.name = name;
				conf.resolution = Vec3i(camera.resolution, 1);
				conf.format = gpu::TextureFormat::RGBA16Float;
				return newTexture(scene.config.shared.device, conf);
			}

			Holder<Texture> createSsaoTextureTarget()
			{
				static constexpr int downscale = 3;
				TransientTextureCreateConfig conf;
				conf.name = "ssao target";
				conf.resolution = Vec3i(camera.resolution, 0) / downscale;
				conf.resolution[2] = 1;
				conf.format = gpu::TextureFormat::R16Float;
				return newTexture(scene.config.shared.device, conf);
			}
		};

		void SceneImpl::prepareEntitiesModelsLod(Entity *e, RenderObject *object)
		{
			const Transform transform = modelTransform(e);
			for (Holder<RenderBaseBase> &renderer : renderers)
			{
				distOverride = +renderer;
				prepareModelsHolders.clear();
				renderer->camera.lodSelection.selectModels(prepareModelsHolders, transform.position, object, config.shared.onDemand);
				for (auto &mesh : prepareModelsHolders)
				{
					SceneModel rm;
					rm.mesh = shareAsset(std::move(mesh));
					SceneItem rd;
					rd.e = e;
					rd.transform = transform;
					rd.data.assign(std::move(rm));
					prepareModel(rd, object);
				}
			}
			distOverride = nullptr;
		}

		void SceneImpl::distribute(SceneItem &&item)
		{
			const auto msk = item.e->getOrDefault<SceneComponent>().sceneMask;
			RenderItem rd{ .base = items.push_back(std::move(item)) };
			if (!rd.base)
				return; // exhausted capacity

			const auto &put = [&](RenderBaseBase *r)
			{
				if (r->isShadowmap && !item.shadowCast)
					return;
				if ((r->camera.cameraSceneMask & msk) == 0)
					return;
				r->items.push_back(rd);
			};

			if (distOverride)
			{
				put(distOverride);
			}
			else
			{
				for (auto &r : renderers)
					put(+r);
			}
		}

		void SceneImpl::generateRenderers()
		{
			const ProfilingScope profiling("generate renderers");

			const uint32 reservation = (config.shared.scene->component<ModelComponent>()->count() + config.shared.scene->component<SpriteComponent>()->count() + config.shared.scene->component<TextComponent>()->count()) * 3 / 2 + 512;
			items.reserve(reservation);
			renderers.reserve(16);

			for (const auto &it : config.cameras)
			{
				Holder<CameraRender> c = systemMemory().createHolder<CameraRender>(*this, it);
				c->items.reserve(reservation);
				c->initialize();
				entitiesVisitor(
					[&](Entity *e, const LightComponent &lc, const ShadowmapComponent &sc)
					{
						if (c->failedMask(e))
							return;
						c->createShadowRender(renderers, e, lc, sc);
					},
					config.shared.scene, false);
				renderers.push_back(std::move(c).cast<RenderBaseBase>());
			}
		}

		void SceneImpl::dispatchRenders()
		{
			tasksRunBlocking<Holder<RenderBaseBase>>("scene render", [](Holder<RenderBaseBase> &it) { it->entry(); }, renderers);
		}
	}

	Holder<PointerRange<Holder<GraphicsEncoder>>> sceneRender(const SceneRenderConfig &config)
	{
		CAGE_ASSERT(config.shared.device);
		CAGE_ASSERT(config.shared.assets);
		CAGE_ASSERT(config.shared.onDemand);
		CAGE_ASSERT(config.shared.scene);
		CAGE_ASSERT(valid(config.shared.interpolationFactor));

		if (!config.shared.assets->get<AssetPack>(HashString("cage/cage.pack")) || !config.shared.assets->get<AssetPack>(HashString("cage/shaders/engine/engine.pack")))
			return {};

		for (const auto &it : config.cameras)
		{
			CAGE_ASSERT(it.target);
			CAGE_ASSERT(it.resolution[0] > 0);
			CAGE_ASSERT(it.resolution[1] > 0);
			CAGE_ASSERT(valid(it.projection));
			CAGE_ASSERT(valid(it.transform));
			CAGE_ASSERT(valid(it.lodSelection.center));
			CAGE_ASSERT(valid(it.lodSelection.screenSize));
		}

		SceneImpl impl(config);
		impl.loadBasicAssets();
		impl.generateRenderers();
		impl.prepareEntities();
		impl.dispatchRenders();

		PointerRangeHolder<Holder<GraphicsEncoder>> res;
		for (auto &it : impl.renderers)
			res.push_back(std::move(it->encoder));
		return res;
	}

	SkeletalAnimationComponent &SkeletalAnimationComponent::clear()
	{
		for (auto &it : animations)
			it = {};
		return *this;
	}

	SkeletalAnimationComponent &SkeletalAnimationComponent::add(SkeletalAnimationLayer layer)
	{
		for (auto &it : animations)
		{
			if (!it.animation)
			{
				it = layer;
				return *this;
			}
		}

		uint32 li = m;
		Real lw = Real::Infinity();
		for (const auto &it : animations)
		{
			if (it.weight < lw)
			{
				li = &it - animations;
				lw = it.weight;
			}
		}
		if (lw < layer.weight)
			animations[li] = layer;
		return *this;
	}

	SkeletalAnimationComponent &SkeletalAnimationComponent::set(uint32 animation)
	{
		clear();
		animations[0].animation = animation;
		return *this;
	}
}
