#include "private.h"

#include <cage-core/assetsManager.h>
#include <cage-core/assetsOnDemand.h>
#include <cage-core/hashString.h>
#include <cage-core/serialization.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-engine/model.h>
#include <cage-engine/opengl.h> // GL_TEXTURE_2D_ARRAY
#include <cage-engine/shaderProgram.h>
#include <cage-engine/texture.h>

namespace cage
{
	void SkinData::bind(RenderQueue *queue) const
	{
		CAGE_ASSERT(texture);
		queue->bind(uubRange, 0);
		queue->bind(texture, 0);
	}

	void RenderableBase::setClip(const HierarchyItem *item)
	{
		clipPos = item->clipPos * item->impl->pointsScale;
		clipSize = item->clipSize * item->impl->pointsScale;
	}

	bool RenderableBase::prepare()
	{
		if (clipSize[0] >= 1 && clipSize[1] >= 1)
		{
			const Vec2i pos = Vec2i(Vec2(clipPos[0], impl->outputResolution[1] - clipPos[1] - clipSize[1]));
			impl->activeQueue->scissors(pos, Vec2i(clipSize));
			return true;
		}
		return false;
	}

	RenderableElement::RenderableElement(WidgetItem *item, GuiElementTypeEnum element, ElementModeEnum mode, Vec2 pos, Vec2 size) : RenderableBase(item->hierarchy->impl)
	{
		CAGE_ASSERT(element < GuiElementTypeEnum::TotalElements);
		CAGE_ASSERT(pos.valid());
		CAGE_ASSERT(size.valid());
		CAGE_ASSERT(item->skin->texture);
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
		RenderQueue *q = impl->activeQueue;
		skin->bind(q);
		Holder<ShaderProgram> shader = impl->graphicsData.elementShader.share();
		q->bind(shader);
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
		e.posOuter = outer;
		e.posInner = inner;
		e.accent = accent;
		e.controlType = element;
		e.layoutMode = (uint32)mode;
		q->universalUniformStruct(e, 2);
		q->draw(impl->graphicsData.elementModel);
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
		data.screenPxRange = data.format.size * pointsScale * 0.13;
	}

	RenderableText::~RenderableText()
	{
		if (!prepare())
			return;
		CAGE_ASSERT(data.font);
		if (data.layout.glyphs.empty())
			return;
		RenderQueue *q = impl->activeQueue;
		Holder<ShaderProgram> shader = impl->graphicsData.fontShader.share();
		q->bind(shader);
		q->uniform(shader, 0, transform);
		q->uniform(shader, 4, data.color);
		q->uniform(shader, 15, data.screenPxRange);
		data.font->render(q, +impl->assetOnDemand, data.layout);
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
		// todo format mode
		this->aniTexFrames = detail::evalSamplesForTextureAnimation(+item->texture, applicationTime(), item->image.animationStart, item->format.animationSpeed, item->format.animationOffset);
	}

	RenderableImage::~RenderableImage()
	{
		if (!prepare())
			return;
		CAGE_ASSERT(texture);
		RenderQueue *q = impl->activeQueue;
		q->bind(texture, 0);
		uint32 hash = 0;
		if (texture->target() == GL_TEXTURE_2D_ARRAY)
			hash += HashString("Animated");
		if (disabled)
			hash += HashString("Disabled");
		Holder<ShaderProgram> shader = impl->graphicsData.imageShader->get(hash);
		q->bind(shader);
		q->uniform(shader, 0, ndcPos);
		q->uniform(shader, 1, uvClip);
		if (texture->target() == GL_TEXTURE_2D_ARRAY)
			q->uniform(shader, 2, aniTexFrames);
		q->draw(impl->graphicsData.imageModel);
	}

	void GuiImpl::GraphicsData::load(AssetsManager *assetMgr)
	{
		const auto &defaultProgram = [](const Holder<MultiShaderProgram> &multi) -> Holder<ShaderProgram>
		{
			if (multi)
				return multi->get(0);
			return {};
		};
		elementShader = defaultProgram(assetMgr->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/gui/element.glsl")));
		fontShader = defaultProgram(assetMgr->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/gui/font.glsl")));
		imageShader = assetMgr->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/gui/image.glsl"));
		colorPickerShader[0] = defaultProgram(assetMgr->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/gui/colorPicker.glsl?F")));
		colorPickerShader[1] = defaultProgram(assetMgr->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/gui/colorPicker.glsl?H")));
		colorPickerShader[2] = defaultProgram(assetMgr->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/gui/colorPicker.glsl?S")));
		elementModel = assetMgr->get<AssetSchemeIndexModel, Model>(HashString("cage/model/guiElement.obj"));
		imageModel = assetMgr->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
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

		struct GraphicsDataCleaner : private Immovable
		{
			GuiImpl *impl = nullptr;

			GraphicsDataCleaner(GuiImpl *impl) : impl(impl) {}

			~GraphicsDataCleaner()
			{
				impl->activeQueue = nullptr;
				impl->graphicsData = GuiImpl::GraphicsData();
				for (auto &it : impl->skins)
					it.texture.clear();
			}
		};
	}

	Holder<RenderQueue> GuiImpl::emit()
	{
		assetOnDemand->process();

		if (outputResolution[0] <= 0 || outputResolution[1] <= 0)
			return {};

		if (!assetMgr->get<AssetSchemeIndexPack, AssetPack>(HashString("cage/cage.pack")))
			return {};

		GraphicsDataCleaner graphicsDataCleaner(this);

		for (auto &s : skins)
		{
			s.texture = assetMgr->get<AssetSchemeIndexTexture, Texture>(s.textureName);
			if (!s.texture)
				return {};
		}

		graphicsData.load(assetMgr);

		Holder<RenderQueue> q = newRenderQueue(Stringizer() + "gui_" + this, provisionalGraphics);

		activeQueue = +q;
		auto namedScope = q->namedScope("gui");

		// write skins uv coordinates
		for (auto &s : skins)
		{
			GuiSkinElementLayout::TextureUv textureUvs[(uint32)GuiElementTypeEnum::TotalElements];
			for (uint32 i = 0; i < (uint32)GuiElementTypeEnum::TotalElements; i++)
				copyTextureUv(s.layouts[i].textureUv, textureUvs[i]);
			s.uubRange = q->universalUniformArray<GuiSkinElementLayout::TextureUv>(textureUvs);
		}

		// render all
		q->scissors(true);
		q->blending(true);
		q->blendFuncAlphaTransparency();
		q->depthTest(false);
		q->viewport(Vec2i(), outputResolution);
		root->childrenEmit();
		q->blending(false);
		q->scissors(false);

		return q;
	}
}
