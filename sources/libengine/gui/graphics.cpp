#include <cage-core/assetManager.h>
#include <cage-core/hashString.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-core/serialization.h>

#include <cage-engine/shaderProgram.h>
#include <cage-engine/texture.h>
#include <cage-engine/model.h>
#include <cage-engine/opengl.h> // GL_TEXTURE_2D_ARRAY

#include "private.h"

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

	RenderableElement::RenderableElement(WidgetItem *item, GuiElementTypeEnum element, uint32 mode, Vec2 pos, Vec2 size) : RenderableBase(item->hierarchy->impl)
	{
		CAGE_ASSERT(element < GuiElementTypeEnum::TotalElements);
		CAGE_ASSERT(mode < 4);
		CAGE_ASSERT(pos.valid());
		CAGE_ASSERT(size.valid());
		CAGE_ASSERT(item->skin->texture);
		setClip(item->hierarchy);
		data.element = (uint32)element;
		data.mode = mode;
		data.outer = item->hierarchy->impl->pointsToNdc(pos, size);
		const Vec4 &border = item->skin->layouts[(uint32)element].border;
		offset(pos, size, -border);
		data.inner = item->hierarchy->impl->pointsToNdc(pos, size);
		skin = item->skin;
	}

	RenderableElement::~RenderableElement()
	{
		if (!prepare())
			return;
		RenderQueue *q = impl->activeQueue;
		skin->bind(q);
		Holder<ShaderProgram> shader = impl->graphicsData.elementShader.share();
		q->bind(shader);
		q->uniform(shader, 0, data.outer);
		q->uniform(shader, 1, data.inner);
		q->uniform(shader, 2, data.element);
		q->uniform(shader, 3, data.mode);
		q->draw(impl->graphicsData.elementModel);
	}

	RenderableText::RenderableText(TextItem *item, Vec2 position, Vec2 size) : RenderableBase(item->hierarchy->impl)
	{
		CAGE_ASSERT(item->color.valid());
		if (!item->font)
			return;
		setClip(item->hierarchy);
		data.Transform = item->Transform;
		data.format = item->format;
		data.font = item->font.share();
		item->glyphs = std::move(item->glyphs);
		data.glyphs = item->glyphs.share();
		data.color = item->color;
		data.cursor = item->cursor;
		data.format.size *= item->hierarchy->impl->pointsScale;
		const Vec2i &orr = item->hierarchy->impl->outputResolution;
		position *= item->hierarchy->impl->pointsScale;
		data.Transform = transpose(Mat4(
			2.0 / orr[0], 0, 0, 2.0 * position[0] / orr[0] - 1.0,
			0, 2.0 / orr[1], 0, 1.0 - 2.0 * position[1] / orr[1],
			0, 0, 1, 0,
			0, 0, 0, 1
		));
		data.format.wrapWidth = size[0] * item->hierarchy->impl->pointsScale;
	}

	RenderableText::~RenderableText()
	{
		if (!prepare())
			return;
		CAGE_ASSERT(data.font);
		if (data.glyphs.empty() && data.cursor != 0)
			return;
		RenderQueue *q = impl->activeQueue;
		Holder<ShaderProgram> shader = impl->graphicsData.fontShader.share();
		q->bind(shader);
		q->uniform(shader, 0, data.Transform);
		q->uniform(shader, 4, data.color);
		data.font->render(q, impl->graphicsData.fontModel, impl->graphicsData.fontShader, data.glyphs, data.format, data.cursor);
	}

	RenderableImage::RenderableImage(ImageItem *item, Vec2 position, Vec2 size) : RenderableBase(item->hierarchy->impl)
	{
		if (!item->texture)
			return;
		setClip(item->hierarchy);
		data.texture = item->texture.share();
		data.ndcPos = item->hierarchy->impl->pointsToNdc(position, size);
		data.uvClip = Vec4(item->image.textureUvOffset, item->image.textureUvOffset + item->image.textureUvSize);
		// todo format mode
		data.aniTexFrames = detail::evalSamplesForTextureAnimation(+item->texture, applicationTime(), item->image.animationStart, item->format.animationSpeed, item->format.animationOffset);
	}

	RenderableImage::~RenderableImage()
	{
		if (!prepare())
			return;
		CAGE_ASSERT(data.texture);
		RenderQueue *q = impl->activeQueue;
		q->bind(data.texture, 0);
		Holder<ShaderProgram> shader = (data.texture->target() == GL_TEXTURE_2D_ARRAY ? impl->graphicsData.imageAnimatedShader : impl->graphicsData.imageStaticShader).share();
		q->bind(shader);
		q->uniform(shader, 0, data.ndcPos);
		q->uniform(shader, 1, data.uvClip);
		if (data.texture->target() == GL_TEXTURE_2D_ARRAY)
			q->uniform(shader, 2, data.aniTexFrames);
		q->draw(impl->graphicsData.imageModel);
	}

	Holder<ShaderProgram> defaultProgram(const Holder<MultiShaderProgram> &multi)
	{
		if (multi)
			return multi->get(0);
		return {};
	}

	void GuiImpl::GraphicsData::load(AssetManager *assetMgr)
	{
		elementShader = defaultProgram(assetMgr->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/gui/element.glsl")));
		fontShader = defaultProgram(assetMgr->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/gui/font.glsl")));
		imageAnimatedShader = defaultProgram(assetMgr->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/gui/image.glsl?A")));
		imageStaticShader = defaultProgram(assetMgr->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/gui/image.glsl?a")));
		colorPickerShader[0] = defaultProgram(assetMgr->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/gui/colorPicker.glsl?F")));
		colorPickerShader[1] = defaultProgram(assetMgr->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/gui/colorPicker.glsl?H")));
		colorPickerShader[2] = defaultProgram(assetMgr->get<AssetSchemeIndexShaderProgram, MultiShaderProgram>(HashString("cage/shader/gui/colorPicker.glsl?S")));
		elementModel = assetMgr->get<AssetSchemeIndexModel, Model>(HashString("cage/model/guiElement.obj"));
		fontModel = assetMgr->get<AssetSchemeIndexModel, Model>(HashString("cage/model/square.obj"));
		imageModel = fontModel.share();
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

			GraphicsDataCleaner(GuiImpl *impl) : impl(impl)
			{}

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
