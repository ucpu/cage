#include <algorithm>
#include <array>
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

		struct SceneObject : private Noncopyable
		{
			RenderObject *object = nullptr;
		};

		enum class VariantEnum : uint8
		{
			None,
			Model,
			Sprite,
			Text,
			Custom,
			Object,
		};

		struct SceneItemVariant : private Immovable
		{
			union Variants
			{
				SceneModel model;
				SceneSprite sprite;
				SceneText text;
				SceneCustom custom;
				SceneObject object;
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

			CAGE_FORCE_INLINE SceneObject &object()
			{
				CAGE_ASSERT(index == VariantEnum::Object);
				return variants.object;
			}

			CAGE_FORCE_INLINE const SceneObject &object() const
			{
				CAGE_ASSERT(index == VariantEnum::Object);
				return variants.object;
			}

			SceneItemVariant() {}

			~SceneItemVariant() { destroy(); }

			void assign(SceneModel &&v)
			{
				destroy();
				new (&variants.model) SceneModel(std::move(v));
				index = VariantEnum::Model;
			}

			void assign(SceneSprite &&v)
			{
				destroy();
				new (&variants.sprite) SceneSprite(std::move(v));
				index = VariantEnum::Sprite;
			}

			void assign(SceneText &&v)
			{
				destroy();
				new (&variants.text) SceneText(std::move(v));
				index = VariantEnum::Text;
			}

			void assign(SceneCustom &&v)
			{
				destroy();
				new (&variants.custom) SceneCustom(std::move(v));
				index = VariantEnum::Custom;
			}

			void assign(SceneObject &&v)
			{
				destroy();
				new (&variants.object) SceneObject(std::move(v));
				index = VariantEnum::Object;
			}

			void destroy() noexcept
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
					case VariantEnum::Object:
						variants.object.~SceneObject();
						break;
				}
				index = VariantEnum::None;
			}
		};

		struct SceneItem : private Noncopyable
		{
			SceneItemVariant data;
			Transform transform;
			Aabb box = Aabb::Universe();
			Vec4 color = Vec4::Nan(); // linear rgb (NOT alpha-premultiplied), opacity
			Vec4 animation = Vec4::Nan(); // time (seconds), speed, offset (normalized), unused
			Entity *e = nullptr;
			sint32 renderLayer = 0;
			bool translucent = false; // transparent or fade

			SceneItem() = default;

			SceneItem(SceneItem &&other) noexcept
			{
				// performance hack
				detail::memcpy(this, &other, sizeof(SceneItem));
				//detail::memset(&other, 0, sizeof(SceneItem));
				other.data.index = VariantEnum::None;
			}

			SceneItem &operator=(SceneItem &&other) noexcept
			{
				// performance hack
				if (&other == this)
					return *this;
				this->~SceneItem();
				detail::memcpy(this, &other, sizeof(SceneItem));
				//detail::memset(&other, 0, sizeof(SceneItem));
				other.data.index = VariantEnum::None;
				return *this;
			}

			// approximate order to improve cache locality while rendering
			std::tuple<sint32, bool, void *, void *, bool> cmp() const noexcept
			{
				switch (data.index)
				{
					case VariantEnum::None:
						return {};
					case VariantEnum::Model:
						return { renderLayer, translucent, data.model().mesh, nullptr, !!data.model().skeletalAnimation };
					case VariantEnum::Sprite:
						return { renderLayer, translucent, data.sprite().mesh, data.sprite().texture, false };
					case VariantEnum::Text:
						return { renderLayer, translucent, data.text().font, nullptr, false };
					case VariantEnum::Custom:
						return { renderLayer, translucent, nullptr, nullptr, false };
					case VariantEnum::Object:
						return {}; // objects will be converted into models
				}
				return {};
			}
		};

		struct RenderItem
		{
			const SceneItem *base = nullptr;
			Real depth;

			// exact order for correct rendering
			std::tuple<sint32, bool, Real, void *, void *, bool> cmp() const noexcept
			{
				switch (base->data.index)
				{
					case VariantEnum::None:
						return {};
					case VariantEnum::Model:
						return { base->renderLayer, base->translucent, depth, base->data.model().mesh, nullptr, !!base->data.model().skeletalAnimation };
					case VariantEnum::Sprite:
						return { base->renderLayer, base->translucent, depth, base->data.sprite().mesh, base->data.sprite().texture, false };
					case VariantEnum::Text:
						return { base->renderLayer, base->translucent, depth, base->data.text().font, nullptr, false };
					case VariantEnum::Custom:
						return { base->renderLayer, base->translucent, depth, nullptr, nullptr, false };
					case VariantEnum::Object:
						return {}; // objects are already converted into models
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
						return { d.data.model().mesh, nullptr, !!d.data.model().skeletalAnimation, d.translucent };
					case VariantEnum::Sprite:
						return { d.data.sprite().mesh, d.data.sprite().texture, false, d.translucent };
					case VariantEnum::Text:
						return { d.data.text().font, nullptr, false, d.translucent };
					case VariantEnum::Custom:
						return { d.e, nullptr, false, d.translucent };
					case VariantEnum::Object:
						return {}; // objects are already converted into models
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

		CAGE_FORCE_INLINE AnimationSpeedComponent getAnimSpeed(Entity *e)
		{
			AnimationSpeedComponent anim = e->getOrDefault<AnimationSpeedComponent>();
			if (!anim.speed.valid())
				anim.speed = 1;
			if (!anim.offset.valid())
				anim.offset = 0;
			return anim;
		}

		CAGE_FORCE_INLINE UniMesh makeMeshUni(const SceneItem &rd)
		{
			UniMesh uni;
			uni.modelMat = Mat3x4(Mat4(rd.transform));
			uni.color = rd.color;
			uni.animation = rd.animation;
			return uni;
		}

		CAGE_FORCE_INLINE Transform approximateMatrix(Mat4 mat)
		{
			const Vec3 center = Vec3(mat * Vec4(0, 0, 0, 1));
			const Vec3 forward = Vec3(mat * Vec4(0, 0, -1, 1));
			const Vec3 up = Vec3(mat * Vec4(0, 1, 0, 1));
			return Transform(center, Quat(forward - center, up - center), distance(center, forward));
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

		Holder<Texture> createShadowmap2D(uint32 entityId, uint32 resolution, uint32 cascades, GraphicsDevice *device)
		{
			TransientTextureCreateConfig conf;
			conf.name = "shadowmap target";
			conf.resolution = Vec3i(resolution, resolution, cascades);
			conf.format = wgpu::TextureFormat::Depth32Float;
			conf.flags = TextureFlags::Array;
			conf.entityId = entityId;
			return newTexture(device, conf);
		}

		Holder<Texture> createShadowmapCube(uint32 entityId, uint32 resolution, GraphicsDevice *device)
		{
			TransientTextureCreateConfig conf;
			conf.name = "shadowmap target";
			conf.resolution = Vec3i(resolution, resolution, 6);
			conf.format = wgpu::TextureFormat::Depth16Unorm;
			conf.flags = TextureFlags::Cubemap;
			conf.entityId = entityId;
			return newTexture(device, conf);
		}

		struct SharedCommon : private Noncopyable
		{
			ankerl::unordered_dense::set<Holder<void>> sharedAssetsCache;

			template<class T>
			CAGE_FORCE_INLINE T *shareAsset(Holder<T> &&asset)
			{
				T *ret = +asset;
				if (asset)
					sharedAssetsCache.insert(std::move(asset).template cast<void>());
				return ret;
			}
		};

		struct SceneImpl : public PreparedScene, public SharedCommon
		{
			Model *modelSquare = nullptr, *modelBone = nullptr, *modelSprite = nullptr;
			Shader *shaderBlitPixels = nullptr, *shaderBlitScaled = nullptr;
			MultiShader *shaderStandard = nullptr, *shaderSprite = nullptr;
			Shader *shaderText = nullptr;

			EntityComponent *transformComponent = nullptr;
			EntityComponent *prevTransformComponent = nullptr;

			Holder<SkeletalAnimationPreparatorCollection> skeletonPreparatorCollection;
			const bool cnfRenderMissingModels = confGlobalRenderMissingModels;
			const bool cnfRenderSkeletonBones = confGlobalRenderSkeletonBones;

			std::vector<SceneItem> items;

			// temporary caches
			std::vector<Model *> prepareModelsRaw;

			SceneImpl(const ScenePrepareConfig &config) : PreparedScene{ config }
			{
				skeletonPreparatorCollection = newSkeletalAnimationPreparatorCollection(config.assets);
				transformComponent = config.scene->component<TransformComponent>();
				prevTransformComponent = config.scene->componentsByType(detail::typeIndex<TransformComponent>())[1];
			}

			CAGE_FORCE_INLINE bool emptyMask(Entity *e) const { return e->getOrDefault<SceneComponent>().sceneMask == 0; }

			CAGE_FORCE_INLINE Transform modelTransform(Entity *e) const
			{
				CAGE_ASSERT(e->has(transformComponent));
				if (e->has(prevTransformComponent))
				{
					const Transform c = e->value<TransformComponent>(transformComponent);
					const Transform p = e->value<TransformComponent>(prevTransformComponent);
					return interpolate(p, c, config.interpolationFactor);
				}
				else
					return e->value<TransformComponent>(transformComponent);
			}

			void loadBasicAssets()
			{
				modelSquare = shareAsset(config.assets->get<Model>(HashString("cage/models/square.obj")));
				modelBone = shareAsset(config.assets->get<Model>(HashString("cage/models/bone.obj")));
				modelSprite = shareAsset(config.assets->get<Model>(HashString("cage/models/icon.obj")));
				CAGE_ASSERT(modelSquare && modelBone && modelSprite);

				shaderBlitPixels = shareAsset(config.assets->get<MultiShader>(HashString("cage/shaders/engine/blitPixels.glsl"))->get(0));
				shaderBlitScaled = shareAsset(config.assets->get<MultiShader>(HashString("cage/shaders/engine/blitScaled.glsl"))->get(0));
				CAGE_ASSERT(shaderBlitPixels && shaderBlitScaled);

				shaderStandard = shareAsset(config.assets->get<MultiShader>(HashString("cage/shaders/engine/standard.glsl")));
				shaderSprite = shareAsset(config.assets->get<MultiShader>(HashString("cage/shaders/engine/icon.glsl")));
				CAGE_ASSERT(shaderStandard && shaderSprite);

				shaderText = shareAsset(config.assets->get<MultiShader>(HashString("cage/shaders/engine/text.glsl"))->get(0));
				CAGE_ASSERT(shaderText);
			}

			void prepareModelBones(std::vector<SceneItem> &output, const SceneItem &rd) const
			{
				const SceneModel &rm = rd.data.model();
				CAGE_ASSERT(rm.mesh);
				CAGE_ASSERT(rm.skeletalAnimation);
				const auto armature = rm.skeletalAnimation->armature();
				for (uint32 i = 0; i < armature.size(); i++)
				{
					SceneItem d;
					d.transform = rd.transform * approximateMatrix(Mat4(armature[i]));
					d.box = modelBone->boundingBox * d.transform;
					d.color = Vec4(colorGammaToLinear(colorHsvToRgb(Vec3(Real(i) / Real(armature.size()), 1, 1))), 1);
					d.e = rd.e;
					d.renderLayer = rd.renderLayer;
					SceneModel r;
					r.mesh = modelBone;
					d.data.assign(std::move(r));
					output.push_back(std::move(d));
				}
			}

			void prepareModel(std::vector<SceneItem> &output, SceneItem &rd, const RenderObject *parent = {}) const
			{
				SceneModel &rm = rd.data.model();
				CAGE_ASSERT(rm.mesh);

				std::optional<SkeletalAnimationComponent> ps;
				if (rd.e->has<SkeletalAnimationComponent>())
					ps = rd.e->value<SkeletalAnimationComponent>();

				ModelComponent render = rd.e->value<ModelComponent>();
				ColorComponent color = rd.e->getOrDefault<ColorComponent>();
				const AnimationSpeedComponent anim = getAnimSpeed(rd.e);
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

				if (!rm.mesh->bonesCount)
					ps.reset();
				if (ps)
				{
					if (Holder<SkeletalAnimation> a = config.assets->get<AssetSchemeIndexSkeletalAnimation, SkeletalAnimation>(ps->animation))
					{
						const Real coefficient = detail::evalCoefficientForSkeletalAnimation(+a, config.currentTime, startTime, anim.speed, anim.offset);
						rm.skeletalAnimation = skeletonPreparatorCollection->create(rd.e, std::move(a), coefficient, rm.mesh->importTransform, cnfRenderSkeletonBones);
						rm.skeletalAnimation->prepare();
					}
					else
						ps.reset();
				}

				rd.color = initializeColor(color);
				rd.animation = Vec4((double)(sint64)(config.currentTime - startTime) / (double)1'000'000, anim.speed, anim.offset, 0);
				rd.renderLayer = render.renderLayer + rm.mesh->renderLayer;
				rd.translucent = any(rm.mesh->renderFlags & (MeshRenderFlags::Transparent | MeshRenderFlags::Fade)) || rd.color[3] < 1;
				rd.box = rm.mesh->boundingBox * rd.transform;

				if (rm.skeletalAnimation && cnfRenderSkeletonBones)
					prepareModelBones(output, rd);
				else
					output.push_back(std::move(rd));
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
				rd.box = mesh->boundingBox * rd.transform;
				rd.color = initializeColor(e->getOrDefault<ColorComponent>());
				const uint64 startTime = e->getOrDefault<SpawnTimeComponent>().spawnTime;
				const AnimationSpeedComponent anim = getAnimSpeed(e);
				rd.animation = Vec4((double)(sint64)(config.currentTime - startTime) / (double)1'000'000, anim.speed, anim.offset, 0);
				rd.renderLayer = ic.renderLayer + ri.mesh->renderLayer;
				rd.translucent = true;
				rd.data.assign(std::move(ri));
				items.push_back(std::move(rd));
			}

			void prepareText(Entity *e, TextComponent tc)
			{
				if (!tc.font)
					tc.font = detail::GuiTextFontDefault;
				if (!tc.font)
					tc.font = HashString("cage/fonts/ubuntu/regular.ttf");
				SceneText rt;
				rt.font = shareAsset(config.assets->get<AssetSchemeIndexFont, Font>(tc.font));
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
				rd.translucent = true;
				rd.data.assign(std::move(rt));
				items.push_back(std::move(rd));
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
				rd.translucent = true;
				rd.data.assign(std::move(rc));
				items.push_back(std::move(rd));
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
				data.reserve(config.scene->component<ModelComponent>()->count());

				entitiesVisitor(
					[&](Entity *e, const ModelComponent &rc)
					{
						if (!rc.model)
							return;
						if (emptyMask(e))
							return;
						data.push_back({ e, rc.model });
					},
					+config.scene, false);
				profiling.set(Stringizer() + "models: " + data.size());
				std::sort(data.begin(), data.end(), [](const Data &a, const Data &b) { return a.id < b.id; });

				const auto &mark = [](const Data &data) { return data.id; };
				const auto &output = [&](PointerRange<const Data> data)
				{
					CAGE_ASSERT(data.size() > 0);

					if (Holder<RenderObject> obj = config.assets->get<AssetSchemeIndexRenderObject, RenderObject>(data[0].id))
					{
						CAGE_ASSERT(obj->lodsCount() > 0);
						if (obj->lodsCount() == 1)
						{
							prepareModelsRaw.clear();
							for (uint32 id : obj->models(0))
							{
								if (Model *mesh = shareAsset(config.onDemand->get<AssetSchemeIndexModel, Model>(id)))
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
									prepareModel(items, rd, +obj);
								}
							}
							return;
						}

						// these objects cannot be fully prepared now, we do not have lod selection yet,
						// so we copy the objects as is, and resolve them later in the camera
						RenderObject *o = shareAsset(std::move(obj));
						for (const auto &it : data)
						{
							SceneObject rm;
							rm.object = o;
							SceneItem rd;
							rd.transform = modelTransform(it.e);
							rd.e = it.e;
							rd.data.assign(std::move(rm));
							items.push_back(std::move(rd));
						}
						return;
					}

					if (Model *mesh = shareAsset(config.assets->get<AssetSchemeIndexModel, Model>(data[0].id)))
					{
						for (const auto &it : data)
						{
							SceneModel rm;
							rm.mesh = mesh;
							SceneItem rd;
							rd.e = it.e;
							rd.transform = modelTransform(it.e);
							rd.data.assign(std::move(rm));
							prepareModel(items, rd);
						}
						return;
					}

					if (cnfRenderMissingModels)
					{
						Model *mesh = shareAsset(config.assets->get<AssetSchemeIndexModel, Model>(HashString("cage/models/fake.obj")));
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
							prepareModel(items, rd);
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
				data.reserve(config.scene->component<SpriteComponent>()->count());

				entitiesVisitor(
					[&](Entity *e, const SpriteComponent &ic)
					{
						if (!ic.sprite)
							return;
						if (emptyMask(e))
							return;
						data.push_back({ ic, e });
					},
					+config.scene, false);
				profiling.set(Stringizer() + "sprites: " + data.size());
				std::sort(data.begin(), data.end(), [](const Data &a, const Data &b) { return std::pair{ a.ic.sprite, a.ic.model } < std::pair{ b.ic.sprite, b.ic.model }; });

				const auto &mark = [](const Data &data) { return std::pair{ data.ic.sprite, data.ic.model }; };
				const auto &output = [&](PointerRange<const Data> data)
				{
					CAGE_ASSERT(data.size() > 0);
					Texture *tex = shareAsset(config.assets->get<AssetSchemeIndexTexture, Texture>(data[0].ic.sprite));
					if (!tex)
						return;
					Model *mesh = data[0].ic.model ? shareAsset(config.assets->get<AssetSchemeIndexModel, Model>(data[0].ic.model)) : modelSprite;
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
				profiling.set(Stringizer() + "entities: " + config.scene->count());

				items.reserve(config.scene->component<ModelComponent>()->count() + config.scene->component<SpriteComponent>()->count() + config.scene->component<TextComponent>()->count());

				prepareEntitiesModels();
				prepareEntitiesSprites();

				entitiesVisitor(
					[&](Entity *e, const TextComponent &tc)
					{
						if (emptyMask(e))
							return;
						prepareText(e, tc);
					},
					+config.scene, false);

				entitiesVisitor(
					[&](Entity *e, const CustomDrawComponent &cdc)
					{
						if (emptyMask(e))
							return;
						prepareCustomDraw(e, cdc);
					},
					+config.scene, false);
			}

			void orderSceneData()
			{
				ProfilingScope profiling("order scene data");
				profiling.set(Stringizer() + "count: " + items.size());
				std::sort(items.begin(), items.end(), [](const SceneItem &a, const SceneItem &b) { return a.cmp() < b.cmp(); });
			}
		};

		struct ShadowRender;
		struct CameraRender;

		template<class CrtpDerived>
		struct RenderBase : public SharedCommon
		{
			const SceneCameraConfig config;

			Mat4 model;
			Mat4 view;
			Mat4 viewProj;
			Frustum frustum;
			Holder<GraphicsEncoder> encoder;
			Holder<GraphicsAggregateBuffer> aggregate;
			Holder<GraphicsBuffer> buffViewport, buffProjection;

			const SceneImpl *scene = nullptr;
			std::vector<RenderItem> items;

			// temporary caches
			std::vector<UniMesh> uniMeshes;
			std::vector<Mat3x4> uniArmatures;
			std::vector<float> uniCustomData;

			RenderBase(const SceneCameraConfig &config, const SceneImpl *scene) : config(config), scene(scene) {}

			CAGE_FORCE_INLINE CrtpDerived *derived() { return static_cast<CrtpDerived *>(this); }

			CAGE_FORCE_INLINE bool failedMask(Entity *e) const
			{
				const uint32 c = e->getOrDefault<SceneComponent>().sceneMask;
				return (c & config.cameraSceneMask) == 0;
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

			void renderModels(const RenderModeEnum renderMode, const PointerRange<const RenderItem> instances)
			{
				CAGE_ASSERT(!instances.empty());
				const SceneItem &rd = *instances[0].base;
				const SceneModel &rm = rd.data.model();

				if constexpr (std::is_same_v<CrtpDerived, CameraRender>)
				{
					if (renderMode == RenderModeEnum::DepthPrepass && none(rm.mesh->renderFlags & MeshRenderFlags::DepthWrite))
						return;
				}

				const auto material = newGraphicsBindings(scene->config.device, scene->config.assets, +rm.mesh);

				MultiShader *multiShader = rm.mesh->shaderName ? shareAsset(scene->config.assets->get<AssetSchemeIndexShader, MultiShader>(rm.mesh->shaderName)) : scene->shaderStandard;
				Holder<Shader> shader = pickShaderVariant(multiShader, rm.mesh, textureShaderVariant(material.second), renderMode, !!rm.skeletalAnimation);

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
					const bool ssao = !rd.translucent && any(rm.mesh->renderFlags & MeshRenderFlags::DepthWrite) && any(config.effects.effects & ScreenSpaceEffectsFlags::AmbientOcclusion);
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
					const SceneModel &r = inst.base->data.model();
					CAGE_ASSERT(+r.mesh == +rm.mesh);
					uniMeshes.push_back(makeMeshUni(*inst.base));
					if (rm.skeletalAnimation)
					{
						const auto a = r.skeletalAnimation->armature();
						CAGE_ASSERT(a.size() == rm.mesh->bonesCount);
						for (const auto &b : a)
							uniArmatures.push_back(b);
					}
					appendShaderCustomData(inst.base->e, multiShader->customDataCount);
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
				draw.material = material.first;
				draw.bindings = newGraphicsBindings(scene->config.device, bind);
				draw.instances = instances.size();
				encoder->draw(draw);
			}

			void renderSprites(const RenderModeEnum renderMode, const PointerRange<const RenderItem> instances)
			{
				CAGE_ASSERT(!instances.empty());
				const SceneItem &rd = *instances[0].base;

				if constexpr (std::is_same_v<CrtpDerived, CameraRender>)
				{
					if (renderMode != RenderModeEnum::Color)
						return;
				}

				Model *mesh = rd.data.sprite().mesh;
				Texture *texture = rd.data.sprite().texture;

				MultiShader *multiShader = mesh->shaderName ? shareAsset(scene->config.assets->get<MultiShader>(mesh->shaderName)) : scene->shaderSprite;
				Holder<Shader> shader = pickShaderVariant(multiShader, mesh, textureShaderVariant(texture->flags | (TextureFlags)(1u << 31)), renderMode, false);

				UniOptions uniOptions;

				uniMeshes.clear();
				uniMeshes.reserve(instances.size());
				CAGE_ASSERT(multiShader->customDataCount <= 16);
				uniCustomData.clear();
				if (multiShader->customDataCount)
					uniCustomData.reserve(multiShader->customDataCount * instances.size());

				for (const auto &inst : instances)
				{
					CAGE_ASSERT(inst.base->data.sprite().mesh == mesh);
					CAGE_ASSERT(inst.base->data.sprite().texture == texture);
					uniMeshes.push_back(makeMeshUni(*inst.base));
					appendShaderCustomData(inst.base->e, multiShader->customDataCount);
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
					bind.buffers.push_back({ privat::getBufferDummy(scene->config.device), 2 });
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
				draw.material = newGraphicsBindings(scene->config.device, spritesMaterialBinding(mesh, texture, scene->config.device));
				draw.bindings = newGraphicsBindings(scene->config.device, bind);
				draw.instances = instances.size();
				encoder->draw(draw);
			}

			void renderTexts(const RenderModeEnum renderMode, const PointerRange<const RenderItem> instances)
			{
				CAGE_ASSERT(!instances.empty());

				if constexpr (std::is_same_v<CrtpDerived, CameraRender>)
				{
					if (renderMode != RenderModeEnum::Color)
						return;
				}

				for (const auto &it : instances)
				{
					FontRenderConfig cfg;
					cfg.transform = viewProj * Mat4(it.base->transform);
					cfg.color = it.base->color;
					cfg.encoder = +encoder;
					cfg.aggregate = +aggregate;
					cfg.assets = scene->config.onDemand;
					cfg.depthTest = true;
					cfg.guiShader = false;
					const SceneText &t = it.base->data.text();
					t.font->render(*t.layout, cfg);
				}
			}

			void renderCustom(const RenderModeEnum renderMode, const PointerRange<const RenderItem> instances)
			{
				CAGE_ASSERT(!instances.empty());
				const SceneItem &rd = *instances[0].base;
				rd.data.custom(); // assert the type

				if constexpr (std::is_same_v<CrtpDerived, CameraRender>)
				{
					if (renderMode != RenderModeEnum::Color)
						return;
				}

				for (const auto &it : instances)
				{
					const ProfilingScope profiling("custom draw");
					it.base->data.custom(); // assert type
					CustomDrawConfig cfg;
					cfg.sceneConfig = &scene->config;
					cfg.cameraConfig = &config;
					cfg.entity = it.base->e;
					cfg.encoder = +encoder;
					cfg.aggregate = +aggregate;
					CAGE_ASSERT(it.base->e->has<CustomDrawComponent>());
					it.base->e->value<CustomDrawComponent>().callback(cfg);
				}
			}

			void renderInstances(const RenderModeEnum renderMode, const PointerRange<const RenderItem> instances)
			{
				CAGE_ASSERT(!instances.empty());
				const SceneItem &rd = *instances[0].base;

				if constexpr (std::is_same_v<CrtpDerived, CameraRender>)
				{
					if (renderMode == RenderModeEnum::DepthPrepass && rd.translucent)
						return;
				}

				switch (rd.data.index)
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
					case VariantEnum::Object:
						break; // already converted into models
				}
			}

			void renderPass(const RenderModeEnum renderMode)
			{
				CAGE_ASSERT((std::is_same_v<CrtpDerived, ShadowRender> == (renderMode == RenderModeEnum::Shadowmap)));
				const auto &render = [&](PointerRange<const RenderItem> instances) { renderInstances(renderMode, instances); };
				partition(PointerRange<const RenderItem>(items), [](const RenderItem &r) { return r.mark(); }, render);
			}

			// used for models converted from objects
			void frustumCullRenderData()
			{
				ProfilingScope profiling("frustum cull objects");
				std::erase_if(items,
					[this](const RenderItem &it) -> bool
					{
						if constexpr (std::is_same_v<CrtpDerived, ShadowRender>)
						{
							const auto &mod = it.base->data.model();
							if (none(mod.mesh->renderFlags & MeshRenderFlags::ShadowCast))
								return true;
						}
						if (!intersects(it.base->box, frustum))
							return true;
						return false;
					});
			}

			void orderRenderData()
			{
				ProfilingScope profiling("order render data");
				profiling.set(Stringizer() + "count: " + items.size());
				std::sort(items.begin(), items.end(), [](const RenderItem &a, const RenderItem &b) { return a.cmp() < b.cmp(); });
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

			ShadowRender(const SceneCameraConfig &config, const SceneImpl *scene) : RenderBase(config, scene) {}

			void prepareRenderData()
			{
				ProfilingScope profiling("prepare render data");
				items.reserve(scene->items.size());
				for (const auto &it : scene->items)
				{
					if (it.translucent)
						continue;
					if (failedMask(it.e))
						continue;
					switch (it.data.index)
					{
						case VariantEnum::Model:
						{
							const auto &mod = it.data.model();
							if (none(mod.mesh->renderFlags & MeshRenderFlags::ShadowCast))
								continue;
							break;
						}
						case VariantEnum::Object:
							continue; // already converted to models
						default:
							break;
					}
					if (!intersects(it.box, frustum))
						continue;

					RenderItem r;
					r.base = &it;
					items.push_back(r);
				}
			}

			void initializeShadowmapCascade()
			{
				CAGE_ASSERT(lightComponent.lightType == LightTypeEnum::Directional);
				CAGE_ASSERT(config.camera.shadowmapFrustumDepthFraction > 0 && config.camera.shadowmapFrustumDepthFraction <= 1);

				const auto &sc = shadowmapComponent;
				CAGE_ASSERT(sc.cascadesSplitLogFactor >= 0 && sc.cascadesSplitLogFactor <= 1);
				CAGE_ASSERT(sc.cascadesCount > 0 && sc.cascadesCount <= 4);
				CAGE_ASSERT(sc.cascadesPaddingDistance >= 0);

				const Mat4 invP = inverse(config.projection);
				const auto &getPlane = [&](Real ndcZ) -> Real
				{
					const Vec4 a = invP * Vec4(0, 0, ndcZ, 1);
					return -a[2] / a[3];
				};
				const Real cameraNear = getPlane(0);
				const Real cameraFar = getPlane(1) * config.camera.shadowmapFrustumDepthFraction;
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
					const Vec4 a = config.projection * Vec4(0, 0, -viewZ, 1);
					return a[2] / a[3];
				};
				const Real splitNearNdc = viewZtoNdcZ(splitNear);
				const Real splitFarNdc = viewZtoNdcZ(splitFar);

				const Mat4 invVP = inverse(config.projection * Mat4(inverse(config.transform)));
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

			void taskEntry()
			{
				frustumCullRenderData();
				prepareRenderData();
				orderRenderData();

				encoder = newGraphicsEncoder(scene->config.device, "scene render shadowmap");
				aggregate = newGraphicsAggregateBuffer({ scene->config.device });

				{
					UniProjection uni;
					uni.viewMat = view;
					uni.projMat = shadowProjection;
					uni.vpMat = viewProj;
					uni.vpInv = inverse(viewProj);
					buffProjection = newGraphicsBuffer(scene->config.device, sizeof(uni), "UniProjection");
					buffProjection->writeStruct(uni);
				}

				GraphicsBindings globalBindings;
				{
					GraphicsBuffer *dummyStorage = privat::getBufferDummy(scene->config.device);
					Texture *dummyRegular = privat::getTextureDummy2d(scene->config.device);
					GraphicsBindingsCreateConfig bind;
					bind.buffers.push_back({ .buffer = +buffViewport, .binding = 0, .uniform = true });
					bind.buffers.push_back({ .buffer = +buffProjection, .binding = 1, .uniform = true });
					bind.buffers.push_back({ dummyStorage, 2 });
					bind.buffers.push_back({ dummyStorage, 3 });
					bind.textures.push_back({ dummyRegular, 4 });
					bind.textures.push_back({ dummyRegular, 6 });
					globalBindings = newGraphicsBindings(scene->config.device, bind);
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

			std::vector<SceneItem> objectsLods;

			// temporary caches
			std::vector<Holder<Model>> prepareModelsHolders;

			CameraRender(const SceneCameraConfig &config, const SceneImpl *scene) : RenderBase(config, scene) {}

			void prepareObjectsLods()
			{
				ProfilingScope profiling("prepare objects lods");
				items.reserve(scene->items.size());
				objectsLods.reserve(scene->items.size() / 10);

				for (const auto &it : scene->items)
				{
					if (it.data.index != VariantEnum::Object)
						continue;
					if (failedMask(it.e))
						continue;
					prepareModelsHolders.clear();
					config.lodSelection.selectModels(prepareModelsHolders, it.transform.position, it.data.object().object, scene->config.onDemand);
					for (auto &mesh : prepareModelsHolders)
					{
						SceneModel rm;
						rm.mesh = shareAsset(std::move(mesh));
						SceneItem rd;
						rd.e = it.e;
						rd.transform = it.transform;
						rd.data.assign(std::move(rm));
						scene->prepareModel(objectsLods, rd, it.data.object().object);
					}
				}

				for (const auto &it : objectsLods)
				{
					RenderItem r;
					r.base = &it;
					if (it.translucent)
						r.depth = (viewProj * Vec4(it.transform.position, 1))[2] * -1;
					items.push_back(r);
				}
			}

			void prepareRenderData()
			{
				ProfilingScope profiling("prepare render data");
				items.reserve(scene->items.size());
				for (const auto &it : scene->items)
				{
					if (failedMask(it.e))
						continue;
					if (it.data.index == VariantEnum::Object)
						continue; // already converted to models
					if (!intersects(it.box, frustum))
						continue;

					RenderItem r;
					r.base = &it;
					if (it.translucent)
						r.depth = (viewProj * Vec4(it.transform.position, 1))[2] * -1;
					items.push_back(r);
				}
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
							UniLight uni = initializeLightUni(Mat4(scene->modelTransform(e)), lc, e->getOrDefault<ColorComponent>(), config.transform.position);
							if (lc.lightType == LightTypeEnum::Point && !intersects(Sphere(Vec3(uni.position), lc.maxDistance), frustum))
								return;
							lights.push_back(uni);
						},
						scene->config.scene, false);
					filterLightsOverLimit(lights, config.camera.maxLights);
					lightsCount = numeric_cast<uint32>(lights.size());
					if (lights.empty())
						lights.resize(1); // must ensure non-empty buffer
					buffLights = newGraphicsBuffer(scene->config.device, lights.size() * sizeof(UniLight), "UniLight");
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
					buffShadowedLights = newGraphicsBuffer(scene->config.device, shadows.size() * sizeof(UniShadowedLight), "UniShadowedLight");
					buffShadowedLights->writeArray<UniShadowedLight>(shadows);
				}

				profiling.set(Stringizer() + "count: " + lightsCount + ", shadowed: " + shadowedLightsCount);
			}

			Holder<Texture> createDepthTextureTarget()
			{
				TransientTextureCreateConfig conf;
				conf.name = "depth target";
				conf.resolution = Vec3i(config.resolution, 1);
				conf.format = wgpu::TextureFormat::Depth32Float;
				return newTexture(scene->config.device, conf);
			}

			Holder<Texture> createDepthSamplingTexture()
			{
				TransientTextureCreateConfig conf;
				conf.name = "depth sampling";
				conf.resolution = Vec3i(config.resolution, 1);
				conf.format = wgpu::TextureFormat::R32Float;
				return newTexture(scene->config.device, conf);
			}

			Holder<Texture> createColorTextureTarget(const AssetLabel &name)
			{
				TransientTextureCreateConfig conf;
				conf.name = name;
				conf.resolution = Vec3i(config.resolution, 1);
				conf.format = wgpu::TextureFormat::RGBA16Float;
				return newTexture(scene->config.device, conf);
			}

			Holder<Texture> createSsaoTextureTarget()
			{
				static constexpr int downscale = 3;
				TransientTextureCreateConfig conf;
				conf.name = "ssao target";
				conf.resolution = Vec3i(config.resolution, 0) / downscale;
				conf.resolution[2] = 1;
				conf.format = wgpu::TextureFormat::R16Float;
				return newTexture(scene->config.device, conf);
			}

			void taskEntry()
			{
				frustum = Frustum(viewProj);
				frustumCullRenderData();
				prepareRenderData();
				orderRenderData();
				prepareLights();

				encoder = newGraphicsEncoder(scene->config.device, "scene render camera");
				aggregate = newGraphicsAggregateBuffer({ scene->config.device });

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
							sh2d[i] = privat::getTextureDummyArray(scene->config.device);
						if (!shCube[i])
							shCube[i] = privat::getTextureDummyCube(scene->config.device);
					}

					for (uint32 i = 0; i < 8; i++)
					{
						bind.textures.push_back(GraphicsBindingsCreateConfig::TextureBindingConfig{ .texture = sh2d[i], .binding = 16 + i, .bindSampler = false });
						bind.textures.push_back(GraphicsBindingsCreateConfig::TextureBindingConfig{ .texture = shCube[i], .binding = 24 + i, .bindSampler = false });
					}

					// shadowmap sampler
					bind.textures.push_back(GraphicsBindingsCreateConfig::TextureBindingConfig{ .texture = privat::getTextureShadowsSampler(scene->config.device), .binding = 15, .bindTexture = false });

					globalBindings = newGraphicsBindings(scene->config.device, bind);
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
					drawcfg.model = +scene->modelSquare;
					drawcfg.shader = +scene->shaderBlitPixels;
					GraphicsBindingsCreateConfig bind;
					bind.textures.push_back({ +depthTexture, 0 });
					drawcfg.bindings = newGraphicsBindings(scene->config.device, bind);
					encoder->draw(drawcfg);
				}

				ScreenSpaceCommonConfig commonConfig; // helper to simplify initialization
				commonConfig.assets = scene->config.assets;
				commonConfig.encoder = +encoder;
				commonConfig.aggregate = +aggregate;
				commonConfig.resolution = config.resolution;

				// ssao
				if (any(config.effects.effects & ScreenSpaceEffectsFlags::AmbientOcclusion))
				{
					ScreenSpaceAmbientOcclusionConfig cfg;
					(ScreenSpaceCommonConfig &)cfg = commonConfig;
					(ScreenSpaceAmbientOcclusion &)cfg = config.effects.ssao;
					cfg.proj = config.projection;
					cfg.inDepth = +depthSampling;
					cfg.outAo = +ssaoTexture;
					cfg.frameIndex = scene->config.frameIndex;
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
					if (any(config.effects.effects & ScreenSpaceEffectsFlags::DepthOfField))
					{
						ScreenSpaceDepthOfFieldConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						(ScreenSpaceDepthOfField &)cfg = config.effects.depthOfField;
						cfg.proj = config.projection;
						cfg.inDepth = +depthTexture;
						cfg.inColor = +texSource;
						cfg.outColor = +texTarget;
						screenSpaceDepthOfField(cfg);
						std::swap(texSource, texTarget);
					}

					// bloom
					if (any(config.effects.effects & ScreenSpaceEffectsFlags::Bloom))
					{
						ScreenSpaceBloomConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						(ScreenSpaceBloom &)cfg = config.effects.bloom;
						cfg.inColor = +texSource;
						cfg.outColor = +texTarget;
						screenSpaceBloom(cfg);
						std::swap(texSource, texTarget);
					}

					// tonemap & gamma correction
					if (any(config.effects.effects & (ScreenSpaceEffectsFlags::ToneMapping | ScreenSpaceEffectsFlags::GammaCorrection)))
					{
						ScreenSpaceTonemapConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						cfg.inColor = +texSource;
						cfg.outColor = +texTarget;
						cfg.gamma = any(config.effects.effects & ScreenSpaceEffectsFlags::GammaCorrection) ? config.effects.gamma : 1;
						cfg.tonemapEnabled = any(config.effects.effects & ScreenSpaceEffectsFlags::ToneMapping);
						screenSpaceTonemap(cfg);
						std::swap(texSource, texTarget);
					}

					// fxaa
					if (any(config.effects.effects & ScreenSpaceEffectsFlags::AntiAliasing))
					{
						ScreenSpaceFastApproximateAntiAliasingConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						cfg.inColor = +texSource;
						cfg.outColor = +texTarget;
						screenSpaceFastApproximateAntiAliasing(cfg);
						std::swap(texSource, texTarget);
					}

					// sharpening
					if (any(config.effects.effects & ScreenSpaceEffectsFlags::Sharpening) && config.effects.sharpening.strength > 1e-3)
					{
						ScreenSpaceSharpeningConfig cfg;
						(ScreenSpaceCommonConfig &)cfg = commonConfig;
						cfg.inColor = +texSource;
						cfg.outColor = +texTarget;
						cfg.strength = config.effects.sharpening.strength;
						screenSpaceSharpening(cfg);
						std::swap(texSource, texTarget);
					}

					colorTexture = std::move(texSource);
				}

				// final blit to target

				{
					const ProfilingScope profiling("final blit");
					RenderPassConfig passcfg;
					passcfg.colorTargets.push_back({ config.target });
					encoder->nextPass(passcfg);
					const auto scope = encoder->namedScope("final blit");
					DrawConfig drawcfg;
					drawcfg.model = +scene->modelSquare;
					if (config.resolution == config.target->resolution())
						drawcfg.shader = +scene->shaderBlitPixels;
					else
						drawcfg.shader = +scene->shaderBlitScaled;
					GraphicsBindingsCreateConfig bind;
					bind.textures.push_back({ +colorTexture, 0 });
					drawcfg.bindings = newGraphicsBindings(scene->config.device, bind);
					encoder->draw(drawcfg);
				}

				aggregate->submit();
				aggregate.clear();
			}

			Holder<ShadowRender> createShadowRender(Entity *e, const LightComponent &lc, const ShadowmapComponent &sc)
			{
				CAGE_ASSERT(e->id() != 0); // lights with shadowmap may not be anonymous

				Holder<ShadowRender> data = systemMemory().createHolder<ShadowRender>(config, scene);

				data->items = items; // copy prepared object lods

				data->lightComponent = lc;
				data->shadowmapComponent = sc;
				data->buffViewport = buffViewport.share();

				{ // quantize light direction - reduces shimmering of slowly rotating lights
					CAGE_ASSERT(e->has<TransformComponent>());
					Transform src = scene->modelTransform(e);
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
					(UniLight &)uni = initializeLightUni(data->model, lc, e->getOrDefault<ColorComponent>(), data->config.transform.position);
					uni.shadowParams[2] = sc.normalOffsetScale;
					uni.shadowParams[3] = sc.shadowFactor;
					uni.params[0] += 1; // shadowed light type
				}

				shadowmaps.push_back(data.share());
				return data;
			}

			void prepareShadowmap(std::vector<Holder<AsyncTask>> &outputTasks, Entity *e, const LightComponent &lc, const ShadowmapComponent &sc)
			{
				switch (lc.lightType)
				{
					case LightTypeEnum::Directional:
					{
						CAGE_ASSERT(sc.cascadesCount > 0 && sc.cascadesCount <= 4);
						Holder<Texture> tex = createShadowmap2D(e->id(), sc.resolution, sc.cascadesCount, scene->config.device);
						ShadowRender *first = nullptr;
						for (uint32 cascade = 0; cascade < sc.cascadesCount; cascade++)
						{
							Holder<ShadowRender> data = createShadowRender(e, lc, sc);
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
							outputTasks.push_back(tasksRunAsync<ShadowRender>("render shadowmap cascade", [](ShadowRender &impl, uint32) { impl.taskEntry(); }, std::move(data)));
						}
						break;
					}
					case LightTypeEnum::Spot:
					case LightTypeEnum::Point:
					{
						Holder<ShadowRender> data = createShadowRender(e, lc, sc);
						data->initializeShadowmapSingle();
						if (lc.lightType == LightTypeEnum::Point)
							data->shadowTexture = createShadowmapCube(e->id(), sc.resolution, scene->config.device);
						else
							data->shadowTexture = createShadowmap2D(e->id(), sc.resolution, 1, scene->config.device);
						outputTasks.push_back(tasksRunAsync<ShadowRender>("render shadowmap single", [](ShadowRender &impl, uint32) { impl.taskEntry(); }, std::move(data)));
						break;
					}
					default:
						CAGE_THROW_CRITICAL(Exception, "invalid light type");
				}
			}

			PointerRangeHolder<Holder<GraphicsEncoder>> runEntry()
			{
				PointerRangeHolder<Holder<GraphicsEncoder>> result;

				model = Mat4(config.transform);
				view = inverse(model);
				viewProj = config.projection * view;

				{
					UniViewport viewport;
					viewport.viewMat = view;
					viewport.eyePos = model * Vec4(0, 0, 0, 1);
					viewport.eyeDir = model * Vec4(0, 0, -1, 0);
					viewport.ambientLight = Vec4(colorGammaToLinear(config.camera.ambientColor) * config.camera.ambientIntensity, 0);
					viewport.skyLight = Vec4(colorGammaToLinear(config.camera.skyColor) * config.camera.skyIntensity, 0);
					viewport.viewport = Vec4(Vec2(), Vec2(config.resolution));
					viewport.time = Vec4(scene->config.frameIndex % 10'000, (scene->config.currentTime % uint64(1e6)) / 1e6, (scene->config.currentTime % uint64(1e9)) / 1e9, 0);
					buffViewport = newGraphicsBuffer(scene->config.device, sizeof(viewport), "UniViewport");
					buffViewport->writeStruct(viewport);
				}

				{
					UniProjection uni;
					uni.viewMat = view;
					uni.projMat = config.projection;
					uni.vpMat = viewProj;
					uni.vpInv = inverse(viewProj);
					buffProjection = newGraphicsBuffer(scene->config.device, sizeof(uni), "UniProjection");
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
						scene->config.scene, false);
					{
						const ProfilingScope profiling("execute camera");
						taskEntry();
					}
					{
						const ProfilingScope profiling("waiting for shadowmaps");
						for (const auto &it : tasks)
							it->wait();
					}
				}

				for (const auto &it : shadowmaps)
					result.push_back(std::move(it->encoder));
				result.push_back(std::move(encoder));
				return result;
			}
		};
	}

	Holder<PreparedScene> scenePrepare(const ScenePrepareConfig &config)
	{
		CAGE_ASSERT(config.device);
		CAGE_ASSERT(config.assets);
		CAGE_ASSERT(config.onDemand);
		CAGE_ASSERT(config.scene);
		CAGE_ASSERT(valid(config.interpolationFactor));

		if (!config.assets->get<AssetPack>(HashString("cage/cage.pack")) || !config.assets->get<AssetPack>(HashString("cage/shaders/engine/engine.pack")))
			return {};

		ProfilingScope profiling("scene prepare");
		Holder<SceneImpl> scene = systemMemory().createHolder<SceneImpl>(config);
		scene->loadBasicAssets();
		scene->prepareEntities();
		scene->orderSceneData();
		return std::move(scene).cast<PreparedScene>();
	}

	Holder<PointerRange<Holder<GraphicsEncoder>>> sceneRender(const PreparedScene *scene, const SceneCameraConfig &config)
	{
		CAGE_ASSERT(config.target);
		CAGE_ASSERT(config.resolution[0] > 0);
		CAGE_ASSERT(config.resolution[1] > 0);
		CAGE_ASSERT(valid(config.projection));
		CAGE_ASSERT(valid(config.transform));
		CAGE_ASSERT(valid(config.lodSelection.center));
		CAGE_ASSERT(valid(config.lodSelection.screenSize));

		if (!scene)
			return {};

		ProfilingScope profiling("scene render");
		Holder<CameraRender> camera = systemMemory().createHolder<CameraRender>(config, static_cast<const SceneImpl *>(scene));
		camera->prepareObjectsLods();
		return camera->runEntry();
	}
}
