#include "private.h"

#include <cage-core/assetsManager.h>
#include <cage-core/assetsOnDemand.h>
#include <cage-core/hashString.h>
#include <cage-core/memoryAllocators.h>
#include <cage-core/profiling.h>
#include <cage-core/scopeGuard.h>
#include <cage-engine/graphicsAggregateBuffer.h>
#include <cage-engine/graphicsBindings.h> // prepareModelBindings
#include <cage-engine/graphicsBuffer.h>
#include <cage-engine/graphicsEncoder.h>
#include <cage-engine/model.h>
#include <cage-engine/shader.h>
#include <cage-engine/texture.h>

namespace cage
{
	struct AssetPack;

	void RenderableBase::setClip(const HierarchyItem *item)
	{
		clipPos = item->clipPos * item->impl->pointsScale;
		clipSize = item->clipSize * item->impl->pointsScale;
	}

	bool RenderableBase::prepare()
	{
		if (clipSize[0] >= 1 && clipSize[1] >= 1)
		{
			class Command : public GuiRenderCommandBase
			{
			public:
				Vec4i scissors;

				Command(Vec4i scissors) : scissors(scissors) {}

				void draw(const GuiRenderConfig &config) override { config.encoder->nativeRenderEncoder().SetScissorRect(scissors[0], scissors[1], scissors[2], scissors[3]); }
			};

			const Vec4i scissors = Vec4i(Vec2i(clipPos), Vec2i(clipSize));
			impl->activeQueue->commands.push_back(impl->activeQueue->memory->createImpl<GuiRenderCommandBase, Command>(scissors));
			return true;
		}
		return false;
	}

	RenderableElement::RenderableElement(WidgetItem *item, GuiElementTypeEnum element, ElementModeEnum mode, Vec2 pos, Vec2 size) : RenderableBase(item->hierarchy->impl)
	{
		CAGE_ASSERT(element < GuiElementTypeEnum::TotalElements);
		CAGE_ASSERT(pos.valid());
		CAGE_ASSERT(size.valid());
		setClip(item->hierarchy);
		this->element = (uint32)element;
		this->mode = mode;
		this->outer = item->hierarchy->impl->pointsToNdc(pos, size);
		const Vec4 &border = item->skin->layouts[(uint32)element].border;
		offset(pos, size, -border);
		this->inner = item->hierarchy->impl->pointsToNdc(pos, size);
		this->accent = item->widgetState.accent;
		this->skin = item->skin;
	}

	RenderableElement::~RenderableElement()
	{
		if (!prepare())
			return;

		class Command : public GuiRenderCommandBase
		{
		public:
			struct ElementStruct
			{
				Vec4 posOuter;
				Vec4 posInner;
				Vec4 accent;
				uint32 controlType = 0;
				uint32 layoutMode = 0;
				uint32 dummy1 = 0;
				uint32 dummy2 = 0;
			};
			ElementStruct e;
			DrawConfig drw;
			const GuiRenderImpl::SkinData *skin = nullptr;

			Command(RenderableElement *base)
			{
				GuiRenderImpl *activeQueue = base->impl->activeQueue;
				skin = &activeQueue->findSkin(base->skin);

				e.posOuter = base->outer;
				e.posInner = base->inner;
				e.accent = base->accent;
				e.controlType = base->element;
				e.layoutMode = (uint32)base->mode;

				drw.shader = +activeQueue->elementShader;
				drw.model = +activeQueue->elementModel;
				drw.depthTest = DepthTestEnum::Always;
				drw.depthWrite = false;
				drw.blending = BlendingEnum::AlphaTransparency;
			}

			void draw(const GuiRenderConfig &config) override
			{
				const auto ab = config.aggregate->writeStruct(e, 0, true);
				GraphicsBindingsCreateConfig bind;
				bind.buffers.push_back(ab);
				bind.buffers.push_back(skin->uvsBinding);
				bind.textures.push_back({ +skin->texture, 2 });
				drw.dynamicOffsets.clear();
				drw.dynamicOffsets.push_back(ab);
				drw.dynamicOffsets.push_back(skin->uvsBinding);
				drw.bindings = newGraphicsBindings(config.device, bind);
				config.encoder->draw(drw);
			}
		};

		impl->activeQueue->commands.push_back(impl->activeQueue->memory->createImpl<GuiRenderCommandBase, Command>(this));
	}

	RenderableText::RenderableText(TextItem *item, Vec2 position, Vec2 size, bool disabled) : RenderableBase(item->hierarchy->impl)
	{
		CAGE_ASSERT(item->color.valid());
		if (!item->font)
			return;
		setClip(item->hierarchy);
		data.format = item->format;
		data.font = item->font.share();
		data.layout.glyphs = item->layout.glyphs.share();
		data.layout.size = item->layout.size;
		data.layout.cursor = item->layout.cursor;
		data.color = item->color;
		if (disabled)
		{
			static constexpr Vec3 lum = Vec3(0.299, 0.587, 0.114);
			Real bw = dot(lum, data.color); // desaturate
			bw = (bw - 0.5) * 0.7 + 0.5; // reduce contrast
			data.color = Vec3(bw);
		}
		const Vec2i orr = item->hierarchy->impl->outputResolution;
		const Real pointsScale = item->hierarchy->impl->pointsScale;
		position *= pointsScale;
		transform = transpose(Mat4(pointsScale * 2.0 / orr[0], 0, 0, 2.0 * position[0] / orr[0] - 1.0, 0, pointsScale * 2.0 / orr[1], 0, 1.0 - 2.0 * position[1] / orr[1], 0, 0, 1, 0, 0, 0, 0, 1));
	}

	RenderableText::~RenderableText()
	{
		if (!prepare())
			return;
		CAGE_ASSERT(data.font);
		if (data.layout.glyphs.empty())
			return;

		class Command : public GuiRenderCommandBase
		{
		public:
			CommonTextData data;
			FontRenderConfig cfg;

			Command(RenderableText *base) : data(std::move(base->data))
			{
				cfg.transform = base->transform;
				cfg.color = Vec4(data.color, 1);
				cfg.assets = +base->impl->assetOnDemand;
				cfg.depthTest = false;
			}

			void draw(const GuiRenderConfig &config) override
			{
				auto &c = const_cast<FontRenderConfig &>(cfg);
				c.encoder = config.encoder;
				c.aggregate = config.aggregate;
				data.font->render(data.layout, cfg);
			}
		};

		impl->activeQueue->commands.push_back(impl->activeQueue->memory->createImpl<GuiRenderCommandBase, Command>(this));
	}

	namespace
	{
		void evaluateImageMode(Vec4 &ndc, Vec2i screenSize, Vec4 &uv, Vec2i textureSize, ImageModeEnum mode)
		{
			CAGE_ASSERT(mode != ImageModeEnum::None);

			const Vec2 scr = Vec2(ndc[2] - ndc[0], ndc[3] - ndc[1]) * Vec2(screenSize);
			CAGE_ASSERT(valid(scr));
			if (scr[0] < 1 || scr[1] < 1)
				return;
			const Real st = scr[0] / scr[1];
			CAGE_ASSERT(valid(st) && st > 0);

			const Vec2 tex = Vec2(uv[2] - uv[0], uv[3] - uv[1]) * Vec2(textureSize);
			CAGE_ASSERT(valid(tex));
			const Real at = abs(tex[0] / tex[1]);
			CAGE_ASSERT(valid(at) && at > 0);

			switch (mode)
			{
				case ImageModeEnum::None:
				case ImageModeEnum::Stretch:
					break;

				case ImageModeEnum::Fill:
				{
					// todo
					break;
				}

				case ImageModeEnum::Fit:
				{
					if (st > at)
					{ // preserve height, adjust width
						Real c = (ndc[2] + ndc[0]) * 0.5;
						Real w = ndc[2] - ndc[0];
						w *= 0.5 * at / st;
						ndc[0] = c - w;
						ndc[2] = c + w;
					}
					else
					{ // preserve width, adjust height
						Real c = (ndc[3] + ndc[1]) * 0.5;
						Real h = ndc[3] - ndc[1];
						h *= 0.5 * at / st;
						ndc[1] = c - h;
						ndc[3] = c + h;
					}
					break;
				}
			}
		}
	}

	RenderableImage::RenderableImage(ImageItem *item, Vec2 position, Vec2 size, bool disabled) : RenderableBase(item->hierarchy->impl)
	{
		if (!item->texture)
			return;
		setClip(item->hierarchy);
		this->texture = item->texture.share();
		this->ndcPos = item->hierarchy->impl->pointsToNdc(position, size);
		this->uvClip = Vec4(item->image.textureUvOffset, item->image.textureUvOffset + item->image.textureUvSize);
		this->disabled = disabled;
		GuiImageFormatComponent format = item->format;
		if (format.mode == ImageModeEnum::None)
			format.mode = ImageModeEnum::Fit;
		if (!valid(format.animationSpeed))
			format.animationSpeed = 1;
		if (!valid(format.animationOffset))
			format.animationOffset = 0;
		evaluateImageMode(this->ndcPos, impl->outputResolution, this->uvClip, this->texture->resolution(), format.mode);
		this->animation = Vec4((double)(sint64)(applicationTime() - item->image.animationStart) / 1'000'000, format.animationSpeed, format.animationOffset, 0);
	}

	RenderableImage::~RenderableImage()
	{
		if (!prepare())
			return;

		class Command : public GuiRenderCommandBase
		{
		public:
			struct UniData
			{
				Vec4 pos; // x1, y1, x2, y2
				Vec4 uv; // x1, y1, x2, y2
				Vec4 animation; // time (seconds), speed, offset (normalized), unused
			};
			UniData data;
			Holder<Texture> texture;
			DrawConfig drw;

			Command(RenderableImage *base) : texture(std::move(base->texture))
			{
				GuiRenderImpl *activeQueue = base->impl->activeQueue;

				data.pos = base->ndcPos;
				data.uv = base->uvClip;
				data.animation = base->animation;

				uint32 hash = 0;
				if (any(texture->flags & TextureFlags::Array))
					hash += HashString("Animated");
				if (any(texture->flags & TextureFlags::Srgb))
					hash += HashString("Delinearize");
				if (base->disabled)
					hash += HashString("Disabled");

				drw.shader = +activeQueue->imageShader->get(hash);
				drw.model = +activeQueue->imageModel;
				drw.depthTest = DepthTestEnum::Always;
				drw.depthWrite = false;
				drw.blending = BlendingEnum::AlphaTransparency;
			}

			void draw(const GuiRenderConfig &config) override
			{
				const auto ab = config.aggregate->writeStruct(data, 0, true);
				GraphicsBindingsCreateConfig bind;
				bind.buffers.push_back(ab);
				bind.textures.push_back({ +texture, 1 });
				drw.dynamicOffsets.clear();
				drw.dynamicOffsets.push_back(ab);
				drw.bindings = newGraphicsBindings(config.device, bind);
				config.encoder->draw(drw);
			}
		};

		impl->activeQueue->commands.push_back(impl->activeQueue->memory->createImpl<GuiRenderCommandBase, Command>(this));
	}

	namespace
	{
		void copyTextureUv(const Vec4 &source, Vec4 &target)
		{
			target[0] = source[0];
			target[2] = source[2];
			target[1] = source[3]; // these are swapped intentionally
			target[3] = source[1];
		}

		void copyTextureUv(const GuiSkinElementLayout::TextureUvOi &source, GuiSkinElementLayout::TextureUvOi &target)
		{
			copyTextureUv(source.inner, target.inner);
			copyTextureUv(source.outer, target.outer);
		}

		void copyTextureUv(const GuiSkinElementLayout::TextureUv &source, GuiSkinElementLayout::TextureUv &target)
		{
			for (int i = 0; i < 4; i++)
				copyTextureUv(source.data[i], target.data[i]);
		}
	}

	GuiRenderImpl::GuiRenderImpl(GuiImpl *impl) : resolution(impl->outputResolution), impl(impl)
	{
		ProfilingScope profiling("GuiRenderImpl");

		AssetsManager *assetMgr = +impl->assetMgr;

		skins.reserve(impl->skins.size());
		for (uint32 i = 0; i < impl->skins.size(); i++)
		{
			SkinData sd;
			sd.texture = assetMgr->get<Texture>(impl->skins[i].textureId);
			if (!sd.texture)
				return;

			// write skins uv coordinates
			for (uint32 e = 0; e < (uint32)GuiElementTypeEnum::TotalElements; e++)
				copyTextureUv(impl->skins[i].layouts[e].textureUv, sd.textureUvs[e]);

			skins.push_back(std::move(sd));
		}

		const auto &defaultProgram = [](const Holder<MultiShader> &multi) -> Holder<Shader>
		{
			if (multi)
				return multi->get(0);
			return {};
		};
		elementShader = defaultProgram(assetMgr->get<MultiShader>(HashString("cage/shaders/gui/element.glsl")));
		elementModel = assetMgr->get<Model>(HashString("cage/models/guiElement.obj"));
		imageShader = assetMgr->get<MultiShader>(HashString("cage/shaders/gui/image.glsl"));
		imageModel = assetMgr->get<Model>(HashString("cage/models/square.obj"));
		colorPickerShader[0] = defaultProgram(assetMgr->get<MultiShader>(HashString("cage/shaders/gui/colorPicker.glsl?F")));
		colorPickerShader[1] = defaultProgram(assetMgr->get<MultiShader>(HashString("cage/shaders/gui/colorPicker.glsl?H")));
		colorPickerShader[2] = defaultProgram(assetMgr->get<MultiShader>(HashString("cage/shaders/gui/colorPicker.glsl?S")));

		prepareModelBindings(+impl->graphicsDevice, assetMgr, +elementModel); // todo remove
		prepareModelBindings(+impl->graphicsDevice, assetMgr, +imageModel);

		memory = newMemoryAllocatorStream({});

		commands.reserve(impl->entities()->count() * 2);
	}

	const GuiRenderImpl::SkinData &GuiRenderImpl::findSkin(const GuiSkinConfig *s) const
	{
		auto idx = s - impl->skins.data();
		CAGE_ASSERT(idx < skins.size());
		return skins[idx];
	}

	Holder<GuiRender> GuiImpl::emit()
	{
		assetOnDemand->process();

		if (outputResolution[0] <= 0 || outputResolution[1] <= 0)
			return {};

		if (!assetMgr->get<AssetPack>(HashString("cage/cage.pack")))
			return {};

		ScopeGuard guard([&]() { activeQueue = nullptr; });

		Holder<GuiRenderImpl> render = systemMemory().createHolder<GuiRenderImpl>(this);
		if (!render->elementShader)
			return {};

		activeQueue = +render;

		{
			ProfilingScope profiling("gui record");
			root->childrenEmit();
			profiling.set(Stringizer() + "commands: " + activeQueue->commands.size());
		}

		render->impl = nullptr; // it shall no longer access the gui

		return std::move(render).cast<GuiRender>();
	}

	void GuiRender::draw(const GuiRenderConfig &config) const
	{
		const ProfilingScope profiling("gui dispatch");
		GuiRenderImpl *impl = (GuiRenderImpl *)this;
		if (impl->resolution != config.resolution)
			return;
		for (auto &it : impl->skins)
			it.uvsBinding = config.aggregate->writeArray<GuiSkinElementLayout::TextureUv>(it.textureUvs, 1, false);
		for (const auto &it : impl->commands)
			it->draw(config);
	}
}
